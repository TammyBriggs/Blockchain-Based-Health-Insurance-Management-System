#include <stdio.h>
#include <string.h>
#include "persistence.h"
#include "chain.h"
#include "state.h"
#include "mempool.h"
#include "insurance.h"
#include "crypto.h"

// Accessing the global state arrays defined in previous files
extern Account accounts[];
extern uint32_t account_count;
extern UTXO utxos[];
extern uint32_t utxo_count;
extern Policy policies[];
extern uint32_t policy_count;

bool save_chain() {
    FILE *file = fopen(DATA_FILE, "wb");
    if (!file) {
        printf("Error: Could not open %s for writing.\n", DATA_FILE);
        return false;
    }

    // Write all states to binary file
    fwrite(&chain_state, sizeof(ChainState), 1, file);
    fwrite(&blockchain, sizeof(Block), chain_state.total_blocks, file);
    
    fwrite(&account_count, sizeof(uint32_t), 1, file);
    fwrite(accounts, sizeof(Account), account_count, file);
    
    fwrite(&utxo_count, sizeof(uint32_t), 1, file);
    fwrite(utxos, sizeof(UTXO), utxo_count, file);
    
    fwrite(&mempool_count, sizeof(uint32_t), 1, file);
    fwrite(mempool, sizeof(MempoolEntry), mempool_count, file);
    
    fwrite(&policy_count, sizeof(uint32_t), 1, file);
    fwrite(policies, sizeof(Policy), policy_count, file);

    fclose(file);
    printf("Blockchain state successfully saved to %s.\n", DATA_FILE);
    return true;
}

bool load_chain() {
    FILE *file = fopen(DATA_FILE, "rb");
    if (!file) {
        printf("No existing chain data found. Initializing fresh blockchain...\n");
        chain_init();
        state_init();
        mempool_init();
        insurance_init();
        return false; // Not an error, just means fresh start
    }

    fread(&chain_state, sizeof(ChainState), 1, file);
    fread(&blockchain, sizeof(Block), chain_state.total_blocks, file);
    
    fread(&account_count, sizeof(uint32_t), 1, file);
    fread(accounts, sizeof(Account), account_count, file);
    
    fread(&utxo_count, sizeof(uint32_t), 1, file);
    fread(utxos, sizeof(UTXO), utxo_count, file);
    
    fread(&mempool_count, sizeof(uint32_t), 1, file);
    fread(mempool, sizeof(MempoolEntry), mempool_count, file);
    
    fread(&policy_count, sizeof(uint32_t), 1, file);
    fread(policies, sizeof(Policy), policy_count, file);

    fclose(file);
    printf("Blockchain state loaded. Running strict verification...\n");
    
    if (!verify_chain()) {
        printf("CRITICAL ERROR: Loaded blockchain failed cryptographic verification! Data corrupted or tampered.\n");
        return false;
    }
    
    printf("Verification passed. System ready.\n");
    return true;
}

bool verify_chain() {
    for (uint32_t i = 0; i < chain_state.total_blocks; i++) {
        Block *b = &blockchain[i];

        // 1. Verify Chain Linkage (previous_hash)
        if (i > 0) {
            unsigned char recomputed_prev_hash[HASH_SIZE];
            hash_block(&blockchain[i - 1], recomputed_prev_hash);
            if (memcmp(b->previous_hash, recomputed_prev_hash, HASH_SIZE) != 0) {
                printf("Verification Failed: Block %u linkage broken.\n", b->block_id);
                return false;
            }
        }

        // 2. Verify Proof-of-Work (Difficulty)
        unsigned char current_hash[HASH_SIZE];
        hash_block(b, current_hash);
        if (!check_difficulty(current_hash, b->difficulty)) {
            printf("Verification Failed: Block %u PoW invalid.\n", b->block_id);
            return false;
        }

        // 3. Verify Merkle Root
        unsigned char recomputed_merkle[HASH_SIZE];
        compute_merkle_root(b->transactions, b->transaction_count, recomputed_merkle);
        if (memcmp(b->merkle_root, recomputed_merkle, HASH_SIZE) != 0) {
            printf("Verification Failed: Block %u Merkle Root mismatch (Transaction altered!).\n", b->block_id);
            return false;
        }

        // 4. Verify ECDSA Signatures
        for (uint32_t j = 0; j < b->transaction_count; j++) {
            if (strcmp(b->transactions[j].sender_address, "SYSTEM_COINBASE") != 0) {
                if (!verify_signature(&b->transactions[j])) {
                    printf("Verification Failed: Block %u contains transaction with invalid signature.\n", b->block_id);
                    return false;
                }
            }
        }
    }
    return true;
}
