#ifndef CRYPTO_H
#define CRYPTO_H

#include "core.h"
#include <openssl/ec.h>

// Key Management
EC_KEY* generate_wallet_keypair();
void get_wallet_address(EC_KEY *key, char *address_out);

// Transaction Signing & Verification
int sign_transaction(Transaction *tx, EC_KEY *private_key);
int verify_signature(const Transaction *tx);

// Helper for Merkle Trees & block hashing (To be expanded in future batches)
void compute_tx_hash(const Transaction *tx, unsigned char *hash_out);

#endif // CRYPTO_H
