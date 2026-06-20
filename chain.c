#include <stdio.h>
#include <string.h>
#include <time.h>
#include <openssl/sha.h>
#include "chain.h"
#include "crypto.h"
#include "mempool.h"
#include "state.h"

Block blockchain[MAX_BLOCKS];
ChainState chain_state;
Token aht_token; // Global instantiation of the Token structure

void chain_init() {
    memset(blockchain, 0, sizeof(blockchain));
    chain_state.current_difficulty = 2; // "2" means hash starts with 00 (hex)
    chain_state.block_reward = 50;      // 50 AHT
    chain_state.total_blocks = 0;
    chain_state.last_retarget_block = 0;

    // Initialize the ALU Health Token (AHT)
    strcpy(aht_token.token_name, "ALU Health Token");
    strcpy(aht_token.token_symbol, "AHT");
    aht_token.total_supply = 1000000000; // 1 Billion Max Supply
}

// Custom Merkle Tree implementation (No external libraries)
void compute_merkle_root(Transaction *txs, uint32_t count, unsigned char *root_out) {
    if (count == 0) {
        memset(root_out, 0, HASH_SIZE);
        return;
    }

    unsigned char hashes[MAX_TRANSACTIONS_PER_BLOCK][HASH_SIZE];
    for (uint32_t i = 0; i < count; i++) {
        compute_tx_hash(&txs[i], hashes[i]); // Hash individual transactions
    }

    uint32_t current_count = count;
    while (current_count > 1) {
        uint32_t next_count = 0;
        for (uint32_t i = 0; i < current_count; i += 2) {
            unsigned char pair[HASH_SIZE * 2];
            memcpy(pair, hashes[i], HASH_SIZE);
            
            if (i + 1 < current_count) {
                memcpy(pair + HASH_SIZE, hashes[i + 1], HASH_SIZE);
            } else {
                // Rule: If odd number of hashes, duplicate the last one
                memcpy(pair + HASH_SIZE, hashes[i], HASH_SIZE); 
            }
            
            SHA256(pair, HASH_SIZE * 2, hashes[next_count]);
            next_count++;
        }
        current_count = next_count;
    }
    memcpy(root_out, hashes[0], HASH_SIZE);
}

// Hash the block header fields
void hash_block(const Block *b, unsigned char *hash_out) {
    char buffer[4096];
    int offset = snprintf(buffer, sizeof(buffer), "%lu%ld%u", b->block_id, (long)b->timestamp, b->transaction_count);
    
    for (int i = 0; i < HASH_SIZE; i++) offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%02x", b->previous_hash[i]);
    for (int i = 0; i < HASH_SIZE; i++) offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%02x", b->merkle_root[i]);
    
    snprintf(buffer + offset, sizeof(buffer) - offset, "%lu%s%u", b->nonce, b->miner_id, b->difficulty);
    SHA256((unsigned char*)buffer, strlen(buffer), hash_out);
}

// Checks if the block hash meets the difficulty target (Leading hex zeros)
bool check_difficulty(const unsigned char *hash, uint32_t difficulty) {
    for (uint32_t i = 0; i < difficulty; i++) {
        uint32_t byte_idx = i / 2;
        bool is_high_nibble = (i % 2 == 0);
        unsigned char actual = is_high_nibble ? (hash[byte_idx] >> 4) : (hash[byte_idx] & 0x0F);
        if (actual != 0) return false;
    }
    return true;
}

// Automatic Difficulty Retargeting (Triggered every 10 blocks)
void retarget_difficulty() {
    if (chain_state.total_blocks < 10 || chain_state.total_blocks % 10 != 0) return;

    uint32_t start_idx = chain_state.total_blocks - 10;
    uint32_t end_idx = chain_state.total_blocks - 1;
    
    time_t total_time = blockchain[end_idx].timestamp - blockchain[start_idx].timestamp;
    time_t average_time = total_time / 10;

    uint32_t old_diff = chain_state.current_difficulty;
    
    if (average_time < 30) {
        chain_state.current_difficulty += 1;
    } else if (average_time > 90) {
        if (chain_state.current_difficulty > 1) {
            chain_state.current_difficulty -= 1;
        }
    }
    
    chain_state.last_retarget_block = chain_state.total_blocks;
    printf("\n[RETARGET] Block %u - Old Diff: %u | New Diff: %u | Avg Time: %ld sec\n", 
           chain_state.total_blocks, old_diff, chain_state.current_difficulty, average_time);
}

// Solo Mining Implementation
bool mine_solo(const char *miner_address) {
    Block new_block;
    memset(&new_block, 0, sizeof(Block));
    
    new_block.block_id = chain_state.total_blocks;
    new_block.timestamp = time(NULL);
    new_block.difficulty = chain_state.current_difficulty;
    strcpy(new_block.miner_id, miner_address);

    // 1. Fetch Transactions from Mempool
    // Leave 1 slot open for the coinbase (mining reward) transaction
    new_block.transaction_count = get_pending_transactions(new_block.transactions, MAX_TRANSACTIONS_PER_BLOCK - 1);
    
    // 2. Add Reward Transaction
    Transaction reward_tx;
    memset(&reward_tx, 0, sizeof(Transaction));
    snprintf(reward_tx.transaction_id, 64, "TX_REWARD_BLK_%lu", new_block.block_id);
    strcpy(reward_tx.sender_address, "SYSTEM_COINBASE");
    strcpy(reward_tx.receiver_address, miner_address);
    reward_tx.amount = chain_state.block_reward;
    reward_tx.transaction_type = TX_TOKEN_TRANSFER;
    reward_tx.timestamp = time(NULL);
    new_block.transactions[new_block.transaction_count++] = reward_tx;

    // 3. Set Previous Hash
    if (chain_state.total_blocks > 0) {
        unsigned char prev_hash[HASH_SIZE];
        hash_block(&blockchain[chain_state.total_blocks - 1], prev_hash);
        memcpy(new_block.previous_hash, prev_hash, HASH_SIZE);
    }

    // 4. Compute Merkle Root
    compute_merkle_root(new_block.transactions, new_block.transaction_count, new_block.merkle_root);

    // 5. Proof-of-Work Loop (Block nonce increments, NOT account nonces)
    unsigned char block_hash[HASH_SIZE];
    new_block.nonce = 0;
    
    printf("Mining Block %lu at Difficulty %u...\n", new_block.block_id, new_block.difficulty);
    while (true) {
        hash_block(&new_block, block_hash);
        if (check_difficulty(block_hash, new_block.difficulty)) {
            break;
        }
        new_block.nonce++; // Increment the BLOCK nonce
    }
    
    printf("Block Mined! Hash found after %lu iterations (nonce).\n", new_block.nonce);

    // 6. Finalize: Save block, update state, clean mempool
    blockchain[chain_state.total_blocks++] = new_block;
    
    // Update Account balances, Nonces, AND UTXO states for all confirmed transactions
    for (uint32_t i = 0; i < new_block.transaction_count; i++) {
        // 1. Account Model Update (Balances)
        update_account_balance(new_block.transactions[i].receiver_address, new_block.transactions[i].amount, true);
        
        if (strcmp(new_block.transactions[i].sender_address, "SYSTEM_COINBASE") != 0) {
            update_account_balance(new_block.transactions[i].sender_address, new_block.transactions[i].amount, false);
            increment_account_nonce(new_block.transactions[i].sender_address); // Replay protection update
            
            // 2. UTXO Model Update (Consumption)
            // The sender must have an unspent UTXO to consume
            consume_any_utxo(new_block.transactions[i].sender_address);
        }
        
        // 3. UTXO Model Update (Creation)
        // Every valid transaction creates a new UTXO for the receiver
        add_utxo(new_block.transactions[i].transaction_id, new_block.transactions[i].receiver_address, new_block.transactions[i].amount);
    }

    remove_confirmed_transactions(new_block.transactions, new_block.transaction_count - 1); // -1 to skip coinbase
    retarget_difficulty();
    return true;
}

// Pool Mining Implementation
bool mine_pool(const char **miner_addresses, const uint32_t *hash_contributions, uint32_t miner_count) {
    if (miner_count == 0 || miner_count >= MAX_TRANSACTIONS_PER_BLOCK) return false;

    Block new_block;
    memset(&new_block, 0, sizeof(Block));
    
    new_block.block_id = chain_state.total_blocks;
    new_block.timestamp = time(NULL);
    new_block.difficulty = chain_state.current_difficulty;
    strcpy(new_block.miner_id, "ALU_MINING_POOL");

    // 1. Fetch Transactions from Mempool
    // Leave room for the multiple reward transactions
    uint32_t max_mempool_txs = MAX_TRANSACTIONS_PER_BLOCK - miner_count;
    new_block.transaction_count = get_pending_transactions(new_block.transactions, max_mempool_txs);

    // 2. Calculate Total Hashes for Proportional Splitting
    uint64_t total_hashes = 0;
    for (uint32_t i = 0; i < miner_count; i++) {
        total_hashes += hash_contributions[i];
    }

    // 3. Generate Separate Reward Transactions for Each Miner
    for (uint32_t i = 0; i < miner_count; i++) {
        // Calculate proportional reward. Multiplication happens first to avoid integer division dropping to 0
        uint64_t reward = (chain_state.block_reward * hash_contributions[i]) / total_hashes;
        
        Transaction reward_tx;
        memset(&reward_tx, 0, sizeof(Transaction));
        snprintf(reward_tx.transaction_id, 64, "TX_POOL_REW_%lu_%u", new_block.block_id, i);
        strcpy(reward_tx.sender_address, "SYSTEM_COINBASE");
        strcpy(reward_tx.receiver_address, miner_addresses[i]);
        reward_tx.amount = reward;
        reward_tx.transaction_type = TX_TOKEN_TRANSFER;
        reward_tx.timestamp = time(NULL);
        
        new_block.transactions[new_block.transaction_count++] = reward_tx;
    }

    // 4. Set Previous Hash
    if (chain_state.total_blocks > 0) {
        unsigned char prev_hash[HASH_SIZE];
        hash_block(&blockchain[chain_state.total_blocks - 1], prev_hash);
        memcpy(new_block.previous_hash, prev_hash, HASH_SIZE);
    }

    // 5. Compute Merkle Root
    compute_merkle_root(new_block.transactions, new_block.transaction_count, new_block.merkle_root);

    // 6. Proof-of-Work Loop
    unsigned char block_hash[HASH_SIZE];
    new_block.nonce = 0;
    
    printf("Pool Mining Block %lu at Difficulty %u...\n", new_block.block_id, new_block.difficulty);
    while (true) {
        hash_block(&new_block, block_hash);
        if (check_difficulty(block_hash, new_block.difficulty)) {
            break;
        }
        new_block.nonce++; 
    }
    
    printf("Block Mined by Pool! Hash found after %lu iterations.\n", new_block.nonce);

    // 7. Finalize: Save block, update state, clean mempool
    blockchain[chain_state.total_blocks++] = new_block;
    
    // Update Account balances, Nonces, AND UTXO states for all confirmed transactions
    for (uint32_t i = 0; i < new_block.transaction_count; i++) {
        // 1. Account Model Update (Balances)
        update_account_balance(new_block.transactions[i].receiver_address, new_block.transactions[i].amount, true);
        
        if (strcmp(new_block.transactions[i].sender_address, "SYSTEM_COINBASE") != 0) {
            update_account_balance(new_block.transactions[i].sender_address, new_block.transactions[i].amount, false);
            increment_account_nonce(new_block.transactions[i].sender_address); // Replay protection update
            
            // 2. UTXO Model Update (Consumption)
            consume_any_utxo(new_block.transactions[i].sender_address);
        }
        
        // 3. UTXO Model Update (Creation)
        add_utxo(new_block.transactions[i].transaction_id, new_block.transactions[i].receiver_address, new_block.transactions[i].amount);
    }

    // Remove only the standard mempool transactions, ignoring the generated coinbase ones
    remove_confirmed_transactions(new_block.transactions, new_block.transaction_count - miner_count);
    retarget_difficulty();
    return true;
}
