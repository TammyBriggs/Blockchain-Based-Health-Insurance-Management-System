#ifndef STATE_H
#define STATE_H

#include "core.h"
#include <stdbool.h>

#define MAX_ACCOUNTS 100
#define MAX_UTXOS 1000

// Account Model Structure
typedef struct {
    char address[130];
    uint64_t balance;
    uint64_t nonce;
} Account;

// UTXO Model Structure
typedef struct {
    char transaction_id[64];
    char receiver_address[130];
    uint64_t amount;
    bool is_spent;
} UTXO;

// Initialization
void state_init();

// --- Account Model Functions ---
Account* get_or_create_account(const char *address);
bool update_account_balance(const char *address, uint64_t amount, bool is_addition);
bool validate_account_nonce(const char *address, uint64_t incoming_nonce);
void increment_account_nonce(const char *address);

// --- UTXO Model Functions ---
bool add_utxo(const char *tx_id, const char *receiver, uint64_t amount);
bool spend_utxo(const char *tx_id, const char *spender_address);
bool is_utxo_spent(const char *tx_id);

#endif // STATE_H
