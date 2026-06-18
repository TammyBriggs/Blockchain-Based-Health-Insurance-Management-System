#include <stdio.h>
#include <string.h>
#include "core.h"
#include "crypto.h"
#include "state.h"

int main() {
    printf("--- Testing Batch 3: Dual-Model State (UTXO & Account) ---\n\n");
    state_init();

    char alice_addr[] = "ALICE_PUB_KEY_HEX";
    char bob_addr[] = "BOB_PUB_KEY_HEX";

    // --- 1. Test Account Model ---
    printf("[Account Model] Funding Alice with 1000 AHT...\n");
    update_account_balance(alice_addr, 1000, true);
    
    Account *alice = get_or_create_account(alice_addr);
    printf("Alice Balance: %lu, Nonce: %lu\n", alice->balance, alice->nonce);

    printf("[Account Model] Alice sends 300 AHT to Bob...\n");
    if (update_account_balance(alice_addr, 300, false)) {
        update_account_balance(bob_addr, 300, true);
        increment_account_nonce(alice_addr);
        printf("Transfer Success. Alice's new nonce: %lu\n", get_or_create_account(alice_addr)->nonce);
    }

    printf("[Account Model] Validating next transaction nonce (Expected 1)...\n");
    if (validate_account_nonce(alice_addr, 1)) {
        printf("Nonce validation: SUCCESS\n\n");
    } else {
        printf("Nonce validation: FAILED\n\n");
    }

    // --- 2. Test UTXO Model ---
    char tx_utxo_id[] = "TX_PREMIUM_001";
    printf("[UTXO Model] Creating UTXO for Bob (Amount: 500)...\n");
    add_utxo(tx_utxo_id, bob_addr, 500);

    printf("[UTXO Model] Bob attempts to spend UTXO %s...\n", tx_utxo_id);
    if (spend_utxo(tx_utxo_id, bob_addr)) {
        printf("Spend: SUCCESS\n");
    }

    printf("[UTXO Model] Bob attempts to double-spend UTXO %s...\n", tx_utxo_id);
    if (spend_utxo(tx_utxo_id, bob_addr)) {
        printf("Double-Spend: SUCCESS (WARNING: BUG)\n");
    } else {
        printf("Double-Spend: BLOCKED (Protection working correctly)\n");
    }

    return 0;
}
