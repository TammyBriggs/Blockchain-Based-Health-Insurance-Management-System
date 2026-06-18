#ifndef MEMPOOL_H
#define MEMPOOL_H

#include "core.h"
#include <stdbool.h>
#include <openssl/ec.h> // Required for EC_KEY

#define MAX_MEMPOOL_SIZE 1000

// Mempool Management
void mempool_init();
bool add_to_mempool(const Transaction *tx, uint64_t fee, MempoolStatus status);
void view_mempool();
void remove_confirmed_transactions(const Transaction *confirmed_txs, uint32_t count);

// Business Logic (Updated to use EC_KEY perfectly matching mempool.c)
bool submit_premium_payment(const char *sender, const char *insurance_pool, const char *reinsurance_pool, uint64_t amount, uint64_t base_fee, EC_KEY *priv_key);

#endif // MEMPOOL_H
