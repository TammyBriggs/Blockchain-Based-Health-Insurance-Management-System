#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "mempool.h"
#include "crypto.h"
#include "state.h"

MempoolEntry mempool[MAX_MEMPOOL_SIZE];
uint32_t mempool_count = 0;

void mempool_init() {
    mempool_count = 0;
    memset(mempool, 0, sizeof(mempool));
}

// Comparator for qsort: Fee descending, then Timestamp ascending
int compare_mempool_entries(const void *a, const void *b) {
    MempoolEntry *entryA = (MempoolEntry *)a;
    MempoolEntry *entryB = (MempoolEntry *)b;

    if (entryA->fee != entryB->fee) {
        // Higher fee comes first
        return (entryB->fee > entryA->fee) ? 1 : -1;
    }
    // If fees are equal, older timestamp comes first
    return (entryA->timestamp > entryB->timestamp) ? 1 : -1;
}

bool add_to_mempool(const Transaction *tx, uint64_t fee, MempoolStatus status) {
    if (mempool_count >= MAX_MEMPOOL_SIZE) return false;

    MempoolEntry *entry = &mempool[mempool_count++];
    strcpy(entry->transaction_id, tx->transaction_id);
    strcpy(entry->sender, tx->sender_address);
    strcpy(entry->receiver, tx->receiver_address);
    entry->amount = tx->amount;
    entry->transaction_type = tx->transaction_type;
    entry->fee = fee;
    entry->timestamp = tx->timestamp;
    entry->status = status;

    // Sort immediately after adding to maintain priority queue
    qsort(mempool, mempool_count, sizeof(MempoolEntry), compare_mempool_entries);
    return true;
}

void view_mempool() {
    printf("\n--- Current Mempool (%u Transactions) ---\n", mempool_count);
    for (uint32_t i = 0; i < mempool_count; i++) {
        printf("[%d] ID: %s | Fee: %lu | Amount: %lu | Type: %d | Status: %d\n",
               i, mempool[i].transaction_id, mempool[i].fee, mempool[i].amount, 
               mempool[i].transaction_type, mempool[i].status);
    }
    printf("-----------------------------------------\n");
}

// Automated business logic: Generates the main premium TX and the 5% reinsurance TX
bool submit_premium_payment(const char *sender, const char *insurance_pool, const char *reinsurance_pool, uint64_t amount, uint64_t base_fee, EC_KEY *priv_key) {
    Transaction primary_tx, reins_tx;
    memset(&primary_tx, 0, sizeof(Transaction));
    memset(&reins_tx, 0, sizeof(Transaction));
    
    uint64_t reinsurance_amount = amount * 0.05; // 5% split
    uint64_t primary_amount = amount - reinsurance_amount;

    // Setup Primary Premium TX
    snprintf(primary_tx.transaction_id, 64, "TX_PREM_%ld", time(NULL));
    strcpy(primary_tx.sender_address, sender);
    strcpy(primary_tx.receiver_address, insurance_pool);
    primary_tx.amount = primary_amount;
    primary_tx.transaction_type = TX_PREMIUM_PAYMENT;
    primary_tx.timestamp = time(NULL);
    
    Account *sender_acc = get_or_create_account(sender);
    primary_tx.sender_nonce = sender_acc->nonce;

    // Setup Reinsurance Contribution TX
    snprintf(reins_tx.transaction_id, 64, "TX_REINS_%ld", time(NULL) + 1);
    strcpy(reins_tx.sender_address, sender);
    strcpy(reins_tx.receiver_address, reinsurance_pool);
    reins_tx.amount = reinsurance_amount;
    reins_tx.transaction_type = TX_REINSURANCE_CONTRIBUTION;
    reins_tx.timestamp = time(NULL) + 1; // Slightly offset timestamp
    reins_tx.sender_nonce = sender_acc->nonce; // Uses same nonce snapshot conceptually before block confirmation

    // Sign both
    if (!sign_transaction(&primary_tx, priv_key) || !sign_transaction(&reins_tx, priv_key)) {
        return false;
    }

    // Add to mempool
    add_to_mempool(&primary_tx, base_fee, STATUS_PENDING);
    add_to_mempool(&reins_tx, base_fee, STATUS_PENDING); // Uses same fee to ensure they travel together

    return true;
}
