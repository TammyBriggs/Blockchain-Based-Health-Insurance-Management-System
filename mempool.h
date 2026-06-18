#ifndef MEMPOOL_H
#define MEMPOOL_H

#include "core.h"
#include <stdbool.h>
#include <openssl/ec.h>

#define MAX_MEMPOOL_SIZE 1000

extern MempoolEntry mempool[MAX_MEMPOOL_SIZE];
extern uint32_t mempool_count; // Expose mempool_count to other files

// Mempool Management
void mempool_init();
bool add_to_mempool(const Transaction *tx, uint64_t fee, MempoolStatus status);
void view_mempool();

// Keep only ONE declaration with the 'const' keyword
void remove_confirmed_transactions(const Transaction *confirmed_txs, uint32_t count);
uint32_t get_pending_transactions(Transaction *out_txs, uint32_t max_count);

// Business Logic
bool submit_premium_payment(const char *sender, const char *insurance_pool, const char *reinsurance_pool, uint64_t amount, uint64_t base_fee, EC_KEY *priv_key);

#endif // MEMPOOL_H
