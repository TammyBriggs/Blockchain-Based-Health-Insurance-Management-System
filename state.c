#include <stdio.h>
#include <string.h>
#include "state.h"

// In-memory state (Will be persisted to disk in Batch 7)
Account accounts[MAX_ACCOUNTS];
uint32_t account_count = 0;

UTXO utxos[MAX_UTXOS];
uint32_t utxo_count = 0;

void state_init() {
    account_count = 0;
    utxo_count = 0;
    memset(accounts, 0, sizeof(accounts));
    memset(utxos, 0, sizeof(utxos));
}

// --- Account Model Implementation ---

Account* get_or_create_account(const char *address) {
    for (uint32_t i = 0; i < account_count; i++) {
        if (strcmp(accounts[i].address, address) == 0) {
            return &accounts[i];
        }
    }
    if (account_count >= MAX_ACCOUNTS) return NULL;
    
    strcpy(accounts[account_count].address, address);
    accounts[account_count].balance = 0;
    accounts[account_count].nonce = 0; // Strictly starts at 0 per instructions
    return &accounts[account_count++];
}

bool update_account_balance(const char *address, uint64_t amount, bool is_addition) {
    Account *acc = get_or_create_account(address);
    if (!acc) return false;

    if (is_addition) {
        acc->balance += amount;
    } else {
        if (acc->balance < amount) return false; // Prevent negative balances
        acc->balance -= amount;
    }
    return true;
}

bool validate_account_nonce(const char *address, uint64_t incoming_nonce) {
    Account *acc = get_or_create_account(address);
    if (!acc) return false;
    
    // Verifying that the transaction's snapshot matches the account's current state
    return (incoming_nonce == acc->nonce); 
}

void increment_account_nonce(const char *address) {
    Account *acc = get_or_create_account(address);
    if (acc) {
        acc->nonce += 1;
    }
}

// --- UTXO Model Implementation ---

bool add_utxo(const char *tx_id, const char *receiver, uint64_t amount) {
    if (utxo_count >= MAX_UTXOS) return false;
    
    strcpy(utxos[utxo_count].transaction_id, tx_id);
    strcpy(utxos[utxo_count].receiver_address, receiver);
    utxos[utxo_count].amount = amount;
    utxos[utxo_count].is_spent = false;
    utxo_count++;
    return true;
}

bool spend_utxo(const char *tx_id, const char *spender_address) {
    for (uint32_t i = 0; i < utxo_count; i++) {
        if (strcmp(utxos[i].transaction_id, tx_id) == 0) {
            
            // UTXO must belong to the spender
            if (strcmp(utxos[i].receiver_address, spender_address) != 0) return false;
            
            // Double-Spending Protection
            if (utxos[i].is_spent) return false; 
            
            utxos[i].is_spent = true;
            return true;
        }
    }
    return false; // UTXO not found
}

bool is_utxo_spent(const char *tx_id) {
    for (uint32_t i = 0; i < utxo_count; i++) {
        if (strcmp(utxos[i].transaction_id, tx_id) == 0) {
            return utxos[i].is_spent;
        }
    }
    return true; // If not found, treat as spent/invalid for safety
}
