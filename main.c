#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "core.h"
#include "crypto.h"
#include "state.h"
#include "mempool.h"
#include "chain.h"
#include "insurance.h"
#include "persistence.h"

#define MAX_ARGS 10

// Global session variables
EC_KEY *session_wallet;
EC_KEY *insurance_wallet;
EC_KEY *reinsurance_wallet;

char session_addr[130];
char ins_addr[130];
char reins_addr[130];

// Expose internal state arrays for CLI history reading
extern Block blockchain[];
extern UTXO utxos[];
extern uint32_t utxo_count;

void print_menu() {
    printf("\n=== ALU Health Insurance Blockchain ===\n");
    printf("\n[Membership Operations]\n  1. wallet_balance\n");
    printf("\n[Policy Operations]\n  2. enroll_policy <plan>\n  3. renew_policy <id>\n");
    printf("\n[Token Operations]\n  4. token_transfer <receiver> <amount>\n");
    printf("\n[Insurance Operations]\n  5. pay_premium <amount> <fee>\n  6. submit_claim <provider> <amount> <pol_id>\n  7. settle_claim <claim_tx_id> <provider_addr> <amount>\n");
    printf("\n[Blockchain Operations]\n  8. mempool_view\n  9. mine_solo\n  10. mine_pool\n  11. blockchain_view\n  12. blockchain_verify\n  13. chain_save\n");
    printf("\n[UTXO Operations]\n  14. utxo_view\n");
    printf("\n[Fraud and Audit Operations]\n  15. fraud_review\n  16. approve_suspicious <tx_id>\n  17. reject_suspicious <tx_id>\n  18. transaction_history\n  19. provider_history <addr>\n  20. premium_history\n  21. claim_history\n");
    printf("\n  0. exit\n=======================================\n");
}

int main() {
    printf("Booting ALU Health Insurance Blockchain...\n");
    
    if (!load_chain()) {
        printf("Starting with empty chain state.\n");
    }

    // Setup wallets for demo routing
    session_wallet = generate_wallet_keypair(); get_wallet_address(session_wallet, session_addr);
    insurance_wallet = generate_wallet_keypair(); get_wallet_address(insurance_wallet, ins_addr);
    reinsurance_wallet = generate_wallet_keypair(); get_wallet_address(reinsurance_wallet, reins_addr);
    
    update_account_balance(session_addr, 10000, true);
    update_account_balance(ins_addr, 50000, true);
    update_account_balance(reins_addr, 50000, true);

    char input[512];
    char *args[MAX_ARGS];

    while (1) {
        print_menu();
        printf("\nALU-Chain> ");
        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "\n")] = 0;

        int argc = 0;
        char *token = strtok(input, " \t");
        while (token != NULL && argc < MAX_ARGS) {
            args[argc++] = token;
            token = strtok(NULL, " \t");
        }
        if (argc == 0) continue; 
        char *cmd = args[0];

        // --- Core Commands ---
        if (strcmp(cmd, "0") == 0 || strcmp(cmd, "exit") == 0) {
            save_chain(); break;
        }
        else if (strcmp(cmd, "1") == 0 || strcmp(cmd, "wallet_balance") == 0) {
            char *addr = (argc > 1) ? args[1] : session_addr;
            Account *acc = get_or_create_account(addr);
            printf("Balance: %lu AHT | Nonce: %lu\n", acc->balance, acc->nonce);
        }
        else if (strcmp(cmd, "2") == 0 || strcmp(cmd, "enroll_policy") == 0) {
            char pol_id[32]; snprintf(pol_id, 32, "POL_%ld", time(NULL));
            if (enroll_policy(pol_id, session_addr, args[1])) printf("Enrolled %s in %s.\n", pol_id, args[1]);
        }
        else if (strcmp(cmd, "3") == 0 || strcmp(cmd, "renew_policy") == 0) {
            if (renew_policy(args[1])) printf("Policy renewed.\n");
        }
        else if (strcmp(cmd, "4") == 0 || strcmp(cmd, "token_transfer") == 0) {
            if (argc < 3) { printf("Usage: 4 <receiver_addr> <amount>\n"); continue; }
            Transaction tx; memset(&tx, 0, sizeof(Transaction));
            snprintf(tx.transaction_id, 64, "TX_TRF_%ld", time(NULL));
            strcpy(tx.sender_address, session_addr);
            strcpy(tx.receiver_address, args[1]);
            tx.amount = strtoull(args[2], NULL, 10);
            tx.transaction_type = TX_TOKEN_TRANSFER;
            tx.timestamp = time(NULL);
            tx.sender_nonce = get_or_create_account(session_addr)->nonce;
            
            if (sign_transaction(&tx, session_wallet)) {
                add_to_mempool(&tx, 15, STATUS_PENDING);
                printf("Transfer signed and queued in mempool.\n");
            }
        }
        else if (strcmp(cmd, "5") == 0 || strcmp(cmd, "pay_premium") == 0) {
            if (submit_premium_payment(session_addr, ins_addr, reins_addr, strtoull(args[1], NULL, 10), strtoull(args[2], NULL, 10), session_wallet)) {
                printf("Premium submitted.\n");
            }
        }
        else if (strcmp(cmd, "6") == 0 || strcmp(cmd, "submit_claim") == 0) {
            if (argc < 4) { printf("Usage: 6 <provider_addr> <amount> <pol_id>\n"); continue; }
            Transaction tx; memset(&tx, 0, sizeof(Transaction));
            snprintf(tx.transaction_id, 64, "TX_CLM_%ld", time(NULL));
            strcpy(tx.sender_address, session_addr);
            strcpy(tx.receiver_address, args[1]);
            tx.amount = strtoull(args[2], NULL, 10);
            tx.transaction_type = TX_CLAIM_SUBMISSION;
            tx.timestamp = time(NULL);
            if (submit_claim(&tx, 10, session_wallet, args[3])) printf("Claim queued.\n");
        }
        else if (strcmp(cmd, "7") == 0 || strcmp(cmd, "settle_claim") == 0) {
            if (argc < 4) { printf("Usage: 7 <claim_tx_id> <provider_addr> <amount>\n"); continue; }
            if (settle_claim(args[1], args[2], strtoull(args[3], NULL, 10), insurance_wallet, reinsurance_wallet, ins_addr, reins_addr)) {
                printf("Settlement processed to mempool.\n");
            }
        }
        else if (strcmp(cmd, "8") == 0 || strcmp(cmd, "mempool_view") == 0) view_mempool();
        else if (strcmp(cmd, "9") == 0 || strcmp(cmd, "mine_solo") == 0) {
            if (mine_solo(session_addr)) { printf("Block mined!\n"); save_chain(); }
        }
        else if (strcmp(cmd, "10") == 0 || strcmp(cmd, "mine_pool") == 0) {
            const char *miners[] = {session_addr, ins_addr};
            uint32_t hashes[] = {7000, 3000};
            if (mine_pool(miners, hashes, 2)) {
                printf("Pool block mined! Rewards split 70/30.\n"); save_chain();
            }
        }
        else if (strcmp(cmd, "11") == 0 || strcmp(cmd, "blockchain_view") == 0) {
            printf("\nBlocks: %u | Difficulty: %u | Reward: %lu\n", chain_state.total_blocks, chain_state.current_difficulty, chain_state.block_reward);
        }
        else if (strcmp(cmd, "12") == 0 || strcmp(cmd, "blockchain_verify") == 0) {
            if (verify_chain()) printf("[SUCCESS] Chain intact.\n");
        }
        else if (strcmp(cmd, "13") == 0 || strcmp(cmd, "chain_save") == 0) save_chain();
        else if (strcmp(cmd, "14") == 0 || strcmp(cmd, "utxo_view") == 0) {
            printf("\n--- UTXO Set ---\n");
            int found = 0;
            for (uint32_t i = 0; i < utxo_count; i++) {
                if (!utxos[i].is_spent) {
                    printf("UTXO ID: %s | Receiver: %s | Amount: %lu\n", utxos[i].transaction_id, utxos[i].receiver_address, utxos[i].amount);
                    found++;
                }
            }
            if (found == 0) printf("No unspent UTXOs available.\n");
        }
        
        // --- Audit & Fraud Commands ---
        else if (strcmp(cmd, "15") == 0 || strcmp(cmd, "fraud_review") == 0) {
            printf("\n--- Suspicious Transactions Pending Review ---\n");
            for (uint32_t i = 0; i < mempool_count; i++) {
                if (mempool[i].status == STATUS_SUSPICIOUS) printf("Suspicious: %s\n", mempool[i].transaction_id);
            }
        }
        else if (strcmp(cmd, "16") == 0 || strcmp(cmd, "approve_suspicious") == 0) {
            if (argc < 2) { printf("Usage: 16 <tx_id>\n"); continue; }
            for (uint32_t i = 0; i < mempool_count; i++) {
                if (strcmp(mempool[i].transaction_id, args[1]) == 0) mempool[i].status = STATUS_PENDING;
            }
        }
        else if (strcmp(cmd, "17") == 0 || strcmp(cmd, "reject_suspicious") == 0) {
            if (argc < 2) { printf("Usage: 17 <tx_id>\n"); continue; }
            Transaction mock; strcpy(mock.transaction_id, args[1]); remove_confirmed_transactions(&mock, 1);
        }
        else if (strcmp(cmd, "18") == 0 || strcmp(cmd, "transaction_history") == 0) {
            printf("\n--- Full On-Chain History ---\n");
            for (uint32_t i = 0; i < chain_state.total_blocks; i++) {
                for (uint32_t j = 0; j < blockchain[i].transaction_count; j++) {
                    printf("Blk %u | TX: %s | Type: %d | Amt: %lu\n", i, blockchain[i].transactions[j].transaction_id, blockchain[i].transactions[j].transaction_type, blockchain[i].transactions[j].amount);
                }
            }
        }
        else if (strcmp(cmd, "19") == 0 || strcmp(cmd, "provider_history") == 0) {
            if (argc < 2) { printf("Usage: 19 <provider_addr>\n"); continue; }
            printf("\n--- Provider History: %s ---\n", args[1]);
            for (uint32_t i = 0; i < chain_state.total_blocks; i++) {
                for (uint32_t j = 0; j < blockchain[i].transaction_count; j++) {
                    if (strcmp(blockchain[i].transactions[j].receiver_address, args[1]) == 0 && 
                       (blockchain[i].transactions[j].transaction_type == TX_CLAIM_SUBMISSION || blockchain[i].transactions[j].transaction_type == TX_CLAIM_SETTLEMENT)) {
                        printf("Blk %u | TX: %s | Amt: %lu\n", i, blockchain[i].transactions[j].transaction_id, blockchain[i].transactions[j].amount);
                    }
                }
            }
        }
        else if (strcmp(cmd, "20") == 0 || strcmp(cmd, "premium_history") == 0) {
            printf("\n--- Premium Payment History ---\n");
            for (uint32_t i = 0; i < chain_state.total_blocks; i++) {
                for (uint32_t j = 0; j < blockchain[i].transaction_count; j++) {
                    if (blockchain[i].transactions[j].transaction_type == TX_PREMIUM_PAYMENT) {
                        printf("Blk %u | TX: %s | Sender: %s | Amt: %lu\n", i, blockchain[i].transactions[j].transaction_id, blockchain[i].transactions[j].sender_address, blockchain[i].transactions[j].amount);
                    }
                }
            }
        }
        else if (strcmp(cmd, "21") == 0 || strcmp(cmd, "claim_history") == 0) {
            printf("\n--- Claim Submission History ---\n");
            for (uint32_t i = 0; i < chain_state.total_blocks; i++) {
                for (uint32_t j = 0; j < blockchain[i].transaction_count; j++) {
                    if (blockchain[i].transactions[j].transaction_type == TX_CLAIM_SUBMISSION) {
                        printf("Blk %u | TX: %s | Provider: %s | Amt: %lu\n", i, blockchain[i].transactions[j].transaction_id, blockchain[i].transactions[j].receiver_address, blockchain[i].transactions[j].amount);
                    }
                }
            }
        }
    }
    EC_KEY_free(session_wallet);
    EC_KEY_free(insurance_wallet);
    EC_KEY_free(reinsurance_wallet);
    return 0;
}
