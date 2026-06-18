#include <stdio.h>
#include <string.h>
#include "insurance.h"
#include "state.h"
#include "mempool.h"
#include "chain.h"
#include "crypto.h"

Policy policies[MAX_POLICIES];
uint32_t policy_count = 0;

void insurance_init() {
    policy_count = 0;
    memset(policies, 0, sizeof(policies));
}

// --- Policy Management ---

bool enroll_policy(const char *policy_id, const char *member_address, const char *plan) {
    if (policy_count >= MAX_POLICIES) return false;
    
    Policy *p = &policies[policy_count++];
    strcpy(p->policy_id, policy_id);
    strcpy(p->member_address, member_address);
    strcpy(p->coverage_plan, plan);
    p->enrollment_date = time(NULL);
    p->expiry_date = p->enrollment_date + (365 * 24 * 60 * 60); // +365 days
    p->status = POLICY_ACTIVE;
    
    return true;
}

Policy* get_policy(const char *policy_id) {
    for (uint32_t i = 0; i < policy_count; i++) {
        if (strcmp(policies[i].policy_id, policy_id) == 0) {
            // Auto-check expiry upon retrieval
            if (policies[i].expiry_date < time(NULL) && policies[i].status != POLICY_EXPIRED) {
                policies[i].status = POLICY_EXPIRED;
            }
            return &policies[i];
        }
    }
    return NULL;
}

bool renew_policy(const char *policy_id) {
    Policy *p = get_policy(policy_id);
    if (!p) return false;
    
    p->expiry_date = time(NULL) + (365 * 24 * 60 * 60);
    p->status = POLICY_RENEWED;
    return true;
}

// --- Fraud Detection Heuristics ---

bool is_duplicate_transaction(const char *tx_id) {
    // 1. Check Mempool
    for (uint32_t i = 0; i < mempool_count; i++) {
        if (strcmp(mempool[i].transaction_id, tx_id) == 0) return true;
    }
    // 2. Check Chain
    for (uint32_t i = 0; i < chain_state.total_blocks; i++) {
        for (uint32_t j = 0; j < blockchain[i].transaction_count; j++) {
            if (strcmp(blockchain[i].transactions[j].transaction_id, tx_id) == 0) return true;
        }
    }
    return false;
}

bool is_high_frequency_claim(const char *member_address, time_t tx_time) {
    int count = 0;
    time_t window_start = tx_time - (24 * 60 * 60); // 24 hours ago
    
    // Check Mempool
    for (uint32_t i = 0; i < mempool_count; i++) {
        if (mempool[i].transaction_type == TX_CLAIM_SUBMISSION && 
            strcmp(mempool[i].sender, member_address) == 0 &&
            mempool[i].timestamp >= window_start) {
            count++;
        }
    }
    // Check Chain
    for (uint32_t i = 0; i < chain_state.total_blocks; i++) {
        for (uint32_t j = 0; j < blockchain[i].transaction_count; j++) {
            if (blockchain[i].transactions[j].transaction_type == TX_CLAIM_SUBMISSION &&
                strcmp(blockchain[i].transactions[j].sender_address, member_address) == 0 &&
                blockchain[i].transactions[j].timestamp >= window_start) {
                count++;
            }
        }
    }
    return count >= 3; // > 3 means if they already have 3, this new one triggers it
}

bool is_abnormal_claim_amount(const char *provider_address, uint64_t amount) {
    uint64_t total_amount = 0;
    uint32_t claim_count = 0;
    
    // Check historical data in chain to establish average
    for (uint32_t i = 0; i < chain_state.total_blocks; i++) {
        for (uint32_t j = 0; j < blockchain[i].transaction_count; j++) {
            if (blockchain[i].transactions[j].transaction_type == TX_CLAIM_SUBMISSION &&
                strcmp(blockchain[i].transactions[j].receiver_address, provider_address) == 0) {
                total_amount += blockchain[i].transactions[j].amount;
                claim_count++;
            }
        }
    }
    
    if (claim_count == 0) return false; // No baseline history yet
    
    uint64_t average = total_amount / claim_count;
    return amount > (2 * average);
}

bool run_fraud_heuristics(const Transaction *tx) {
    if (is_duplicate_transaction(tx->transaction_id)) {
        printf("[FRAUD ALERT] Duplicate Transaction detected: %s\n", tx->transaction_id);
        return true;
    }
    if (tx->transaction_type == TX_CLAIM_SUBMISSION) {
        if (is_high_frequency_claim(tx->sender_address, tx->timestamp)) {
            printf("[FRAUD ALERT] High frequency claims detected for Member.\n");
            return true;
        }
        if (is_abnormal_claim_amount(tx->receiver_address, tx->amount)) {
            printf("[FRAUD ALERT] Abnormal claim amount detected for Provider.\n");
            return true;
        }
    }
    return false;
}

// --- Claim Submission Pipeline ---
bool submit_claim(Transaction *tx, uint64_t fee, EC_KEY *priv_key, const char *policy_id) {
    Policy *p = get_policy(policy_id);
    if (!p || p->status == POLICY_EXPIRED) return false;

    Account *sender_acc = get_or_create_account(tx->sender_address);
    tx->sender_nonce = sender_acc->nonce;
    
    // REPLAY PROTECTION: Validate nonce before signing
    if (!validate_account_nonce(tx->sender_address, tx->sender_nonce)) {
        printf("Transaction Rejected: Invalid nonce (Replay Attack Prevention).\n");
        return false;
    }

    if (!sign_transaction(tx, priv_key)) return false;

    MempoolStatus final_status = run_fraud_heuristics(tx) ? STATUS_SUSPICIOUS : STATUS_PENDING;
    return add_to_mempool(tx, fee, final_status);
}

bool settle_claim(const char *claim_tx_id, const char *provider_addr, uint64_t amount, EC_KEY *ins_priv, EC_KEY *reins_priv, const char *ins_addr, const char *reins_addr) {
    Transaction primary_tx, reins_tx;
    memset(&primary_tx, 0, sizeof(Transaction));
    memset(&reins_tx, 0, sizeof(Transaction));

    if (amount <= 1000) {
        // Insurance pool pays all
        snprintf(primary_tx.transaction_id, 64, "TX_SETTLE_%ld", time(NULL));
        strcpy(primary_tx.sender_address, ins_addr);
        strcpy(primary_tx.receiver_address, provider_addr);
        primary_tx.amount = amount;
        primary_tx.transaction_type = TX_CLAIM_SETTLEMENT;
        primary_tx.timestamp = time(NULL);
        primary_tx.sender_nonce = get_or_create_account(ins_addr)->nonce;
        
        if (!sign_transaction(&primary_tx, ins_priv)) return false;
        return add_to_mempool(&primary_tx, 20, STATUS_PENDING);
    } else {
        // Split logic: Insurance pays 1000, Reinsurance pays the rest
        uint64_t excess = amount - 1000;
        
        snprintf(primary_tx.transaction_id, 64, "TX_SETTLE_MAIN_%ld", time(NULL));
        strcpy(primary_tx.sender_address, ins_addr);
        strcpy(primary_tx.receiver_address, provider_addr);
        primary_tx.amount = 1000;
        primary_tx.transaction_type = TX_CLAIM_SETTLEMENT;
        primary_tx.timestamp = time(NULL);
        primary_tx.sender_nonce = get_or_create_account(ins_addr)->nonce;
        sign_transaction(&primary_tx, ins_priv);
        add_to_mempool(&primary_tx, 20, STATUS_PENDING);

        snprintf(reins_tx.transaction_id, 64, "TX_SETTLE_REINS_%ld", time(NULL) + 1);
        strcpy(reins_tx.sender_address, reins_addr);
        strcpy(reins_tx.receiver_address, provider_addr);
        reins_tx.amount = excess;
        reins_tx.transaction_type = TX_CLAIM_SETTLEMENT;
        reins_tx.timestamp = time(NULL) + 1;
        reins_tx.sender_nonce = get_or_create_account(reins_addr)->nonce;
        sign_transaction(&reins_tx, reins_priv);
        add_to_mempool(&reins_tx, 20, STATUS_PENDING);
        
        printf("Settlement split: 1000 AHT from Primary, %lu AHT from Reinsurance.\n", excess);
        return true;
    }
}
