#ifndef CORE_H
#define CORE_H

#include <stdint.h>
#include <time.h>

#define MAX_TRANSACTIONS_PER_BLOCK 10
#define HASH_SIZE 32 // SHA-256 output size in bytes
#define SIGNATURE_MAX_SIZE 72 // ECDSA DER signature max length

// Transaction Types
typedef enum {
    TX_PREMIUM_PAYMENT,
    TX_REINSURANCE_CONTRIBUTION,
    TX_POLICY_ENROLLMENT,
    TX_POLICY_RENEWAL,
    TX_SERVICE_REQUEST,
    TX_PREAUTH_REQUEST,
    TX_PREAUTH_APPROVE,
    TX_CLAIM_SUBMISSION,
    TX_CLAIM_APPROVE,
    TX_CLAIM_REJECT,
    TX_CLAIM_SETTLEMENT,
    TX_TOKEN_TRANSFER
} TransactionType;

// Mempool Status
typedef enum {
    STATUS_PENDING,
    STATUS_CONFIRMED,
    STATUS_SUSPICIOUS
} MempoolStatus;

// Token Structure (AHT)
typedef struct {
    char token_name[32];
    char token_symbol[8];
    uint64_t total_supply;
} Token;

// Transaction Structure
// NOTE: Intentionally missing a tx_hash field as per instructions.
typedef struct {
    char transaction_id[64]; // UUID or string representation
    char sender_address[130]; // Public key hex string
    char receiver_address[130]; 
    uint64_t amount;
    TransactionType transaction_type;
    time_t timestamp;
    uint64_t sender_nonce; // Snapshot of the account nonce at signing
    unsigned char digital_signature[SIGNATURE_MAX_SIZE]; 
    size_t signature_length;
} Transaction;

// Mempool Entry Structure
typedef struct {
    char transaction_id[64];
    char sender[130];
    char receiver[130];
    uint64_t amount;
    TransactionType transaction_type;
    uint64_t fee; // Priority value
    time_t timestamp; // Needed for secondary sorting
    MempoolStatus status;
    
    // CRITICAL FIX: Retain cryptographic proof for block verification
    uint64_t sender_nonce; 
    unsigned char digital_signature[SIGNATURE_MAX_SIZE]; 
    size_t signature_length;
} MempoolEntry;

// Block Structure
typedef struct {
    uint64_t block_id;
    time_t timestamp;
    uint32_t transaction_count;
    unsigned char previous_hash[HASH_SIZE]; // Forms the chain
    unsigned char merkle_root[HASH_SIZE]; // Summarizes transactions
    uint64_t nonce; // Block-level mining nonce (Proof-of-Work)
    char miner_id[130];
    uint32_t difficulty;
    
    // Transactions are stored in the block but hashed externally for the Merkle Root
    Transaction transactions[MAX_TRANSACTIONS_PER_BLOCK]; 
} Block;

// Blockchain Global State Metadata
typedef struct {
    uint32_t current_difficulty;
    uint64_t block_reward;
    uint32_t total_blocks;
    uint32_t last_retarget_block;
} ChainState;

#endif // CORE_H
