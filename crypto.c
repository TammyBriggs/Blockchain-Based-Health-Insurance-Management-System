#include <stdio.h>
#include <string.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>
#include <openssl/sha.h>
#include "core.h"
#include "crypto.h"

// Generate secp256k1 keypair
EC_KEY* generate_wallet_keypair() {
    EC_KEY *key = EC_KEY_new_by_curve_name(NID_secp256k1);
    if (key == NULL) return NULL;
    
    if (!EC_KEY_generate_key(key)) {
        EC_KEY_free(key);
        return NULL;
    }
    return key;
}

// Extract public key as an uncompressed hex string (serves as wallet address)
void get_wallet_address(EC_KEY *key, char *address_out) {
    const EC_GROUP *group = EC_KEY_get0_group(key);
    const EC_POINT *point = EC_KEY_get0_public_key(key);
    
    char *hex = EC_POINT_point2hex(group, point, POINT_CONVERSION_UNCOMPRESSED, NULL);
    if (hex) {
        strncpy(address_out, hex, 130);
        OPENSSL_free(hex);
    }
}

// Creates the temporary hash for signing or Merkle Tree building
void compute_tx_hash(const Transaction *tx, unsigned char *hash_out) {
    char buffer[1024];
    
    // Concatenate fields into a buffer. 
    // Notice we DO NOT save this hash to the tx struct.
    snprintf(buffer, sizeof(buffer), "%s%s%s%lu%d%ld%lu",
             tx->transaction_id,
             tx->sender_address,
             tx->receiver_address,
             tx->amount,
             tx->transaction_type,
             (long)tx->timestamp,
             tx->sender_nonce);
             
    SHA256((unsigned char*)buffer, strlen(buffer), hash_out);
}

// Sign transaction using the sender's private key
int sign_transaction(Transaction *tx, EC_KEY *private_key) {
    unsigned char temp_hash[SHA256_DIGEST_LENGTH];
    compute_tx_hash(tx, temp_hash);

    unsigned int sig_len;
    // ECDSA_sign produces the DER-encoded digital signature
    if (ECDSA_sign(0, temp_hash, SHA256_DIGEST_LENGTH, tx->digital_signature, &sig_len, private_key) != 1) {
        return 0; // Failed to sign
    }
    
    tx->signature_length = sig_len;
    return 1; // Success
}

// Verify transaction signature against the sender's public key (sender_address)
int verify_signature(const Transaction *tx) {
    unsigned char temp_hash[SHA256_DIGEST_LENGTH];
    compute_tx_hash(tx, temp_hash);

    // Reconstruct the OpenSSL public key from the hex string address
    EC_KEY *pub_key = EC_KEY_new_by_curve_name(NID_secp256k1);
    const EC_GROUP *group = EC_KEY_get0_group(pub_key);
    EC_POINT *point = EC_POINT_hex2point(group, tx->sender_address, NULL, NULL);
    
    if (!point) {
        EC_KEY_free(pub_key);
        return 0; // Invalid public key format
    }
    
    EC_KEY_set_public_key(pub_key, point);

    // Verify the signature
    int valid = ECDSA_verify(0, temp_hash, SHA256_DIGEST_LENGTH, tx->digital_signature, tx->signature_length, pub_key);
    
    // Cleanup
    EC_POINT_free(point);
    EC_KEY_free(pub_key);
    
    return valid == 1; // 1 means valid, 0 means tampered/invalid
}
