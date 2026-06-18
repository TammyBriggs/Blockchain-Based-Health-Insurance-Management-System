#include <stdio.h>
#include <string.h>
#include <time.h>
#include "core.h"
#include "crypto.h"
#include "state.h"
#include "mempool.h"
#include "chain.h"

int main() {
    printf("--- Testing Batch 5: Pool Mining & Fixes ---\n\n");
    chain_init();
    state_init();
    mempool_init();

    char miner1[] = "MINER_1_ADDRESS";
    char miner2[] = "MINER_2_ADDRESS";
    char miner3[] = "MINER_3_ADDRESS";

    printf("Starting Mempool Count: %u\n", mempool_count);

    // 1. Test Solo Mining
    if (mine_solo(miner1)) {
        printf("Solo Mining Successful! Miner 1 Balance: %lu AHT\n\n", get_or_create_account(miner1)->balance);
    }

    // 2. Submit a dummy transaction for the next block
    Transaction dummy_tx;
    memset(&dummy_tx, 0, sizeof(Transaction));
    strcpy(dummy_tx.transaction_id, "TX_USER_POOL_TEST");
    strcpy(dummy_tx.sender_address, "SOME_USER_ADDRESS");
    strcpy(dummy_tx.receiver_address, "PROVIDER_ADDRESS");
    dummy_tx.amount = 100;
    dummy_tx.timestamp = time(NULL);
    add_to_mempool(&dummy_tx, 15, STATUS_PENDING);

    // 3. Test Pool Mining (Simulating hash contributions)
    printf("Simulating Pool Mining with 3 Miners...\n");
    printf("Miner 1 contributed 5000 hashes\n");
    printf("Miner 2 contributed 3000 hashes\n");
    printf("Miner 3 contributed 2000 hashes\n");
    
    const char *pool_miners[] = {miner1, miner2, miner3};
    uint32_t contributions[] = {5000, 3000, 2000}; // Total 10,000 hashes. Block reward is 50.

    if (mine_pool(pool_miners, contributions, 3)) {
        printf("\nPool Mining Successful!\n");
        printf("Miner 1 Total Balance: %lu AHT (Expected: 50 from solo + 25 from pool = 75)\n", get_or_create_account(miner1)->balance);
        printf("Miner 2 Total Balance: %lu AHT (Expected: 15)\n", get_or_create_account(miner2)->balance);
        printf("Miner 3 Total Balance: %lu AHT (Expected: 10)\n", get_or_create_account(miner3)->balance);
        printf("Remaining Mempool Count: %u\n", mempool_count);
    }

    return 0;
}
