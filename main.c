#include <stdio.h>
#include <string.h>
#include <time.h>
#include "core.h"
#include "crypto.h"

int main() {
    printf("--- Testing Batch 2: Cryptography & ECDSA ---\n\n");

    // 1. Generate Keypair
    EC_KEY *alice_wallet = generate_wallet_keypair();
    char alice_address[130];
    get_wallet_address(alice_wallet, alice_address);
    printf("Alice Wallet Address (Pub Key):\n%s\n\n", alice_address);

    // 2. Create a Mock Transaction
    Transaction tx;
    memset(&tx, 0, sizeof(Transaction));
    strcpy(tx.transaction_id, "TX_001");
    strcpy(tx.sender_address, alice_address);
    strcpy(tx.receiver_address, "RECEIVER_WALLET_HEX_HERE");
    tx.amount = 500;
    tx.transaction_type = TX_PREMIUM_PAYMENT;
    tx.timestamp = time(NULL);
    tx.sender_nonce = 0; // Account nonce snapshot

    // 3. Sign Transaction
    if (sign_transaction(&tx, alice_wallet)) {
        printf("Transaction signed successfully. Signature Length: %zu bytes\n", tx.signature_length);
    } else {
        printf("Signing failed!\n");
        return 1;
    }

    // 4. Verify Signature (Normal Case)
    if (verify_signature(&tx)) {
        printf("Verification: SUCCESS (Signature is valid)\n");
    } else {
        printf("Verification: FAILED\n");
    }

    // 5. Test Tamper Detection
    printf("\nSimulating network tampering (changing amount from 500 to 5000)...\n");
    tx.amount = 5000; 

    if (verify_signature(&tx)) {
        printf("Verification: SUCCESS (Warning: Tampering went undetected!)\n");
    } else {
        printf("Verification: FAILED (Tampering detected successfully!)\n");
    }

    // Cleanup
    EC_KEY_free(alice_wallet);

    return 0;
}
