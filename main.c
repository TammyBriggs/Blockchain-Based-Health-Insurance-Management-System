#include <stdio.h>
#include <string.h>
#include "core.h"
#include "crypto.h"
#include "state.h"
#include "mempool.h"

int main() {
    printf("--- Testing Batch 4: Mempool & Reinsurance Split ---\n\n");
    state_init();
    mempool_init();

    EC_KEY *alice_wallet = generate_wallet_keypair();
    char alice_addr[130];
    get_wallet_address(alice_wallet, alice_addr);
    
    char insurance_pool[] = "INSURANCE_POOL_HEX";
    char reinsurance_pool[] = "REINSURANCE_POOL_HEX";

    // 1. Give Alice some starting funds
    update_account_balance(alice_addr, 5000, true);

    // 2. Submit a low priority dummy transaction first
    Transaction low_tx;
    memset(&low_tx, 0, sizeof(Transaction));
    strcpy(low_tx.transaction_id, "TX_LOW_PRIORITY");
    strcpy(low_tx.sender_address, alice_addr);
    strcpy(low_tx.receiver_address, "SOME_PROVIDER");
    low_tx.amount = 100;
    low_tx.transaction_type = TX_SERVICE_REQUEST;
    low_tx.timestamp = time(NULL) - 100; // Older timestamp
    add_to_mempool(&low_tx, 10, STATUS_PENDING); // Fee is 10

    // 3. Submit Premium Payment (Triggers the split logic)
    printf("Alice submits a Premium Payment of 1000 AHT with fee 50...\n");
    if (submit_premium_payment(alice_addr, insurance_pool, reinsurance_pool, 1000, 50, alice_wallet)) {
        printf("Premium payment processed into mempool.\n");
    }

    // 4. View Mempool to verify sorting
    // Expected: Premium TX and Reinsurance TX should be at the top (Fee 50), 
    // Low priority TX at the bottom (Fee 10)
    view_mempool();

    EC_KEY_free(alice_wallet);
    return 0;
}
