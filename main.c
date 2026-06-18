#include <stdio.h>
#include <string.h>
#include <time.h>
#include "core.h"
#include "crypto.h"
#include "state.h"
#include "mempool.h"
#include "chain.h"
#include "insurance.h"

int main() {
    printf("--- Testing Batch 6: Policies & Fraud Detection ---\n\n");
    chain_init();
    state_init();
    mempool_init();
    insurance_init();

    EC_KEY *alice_wallet = generate_wallet_keypair();
    char alice_addr[130];
    get_wallet_address(alice_wallet, alice_addr);
    char provider_addr[] = "HOSPITAL_PUB_KEY";

    // 1. Test Policy Enrollment & Expiry Rejection
    printf("[Test 1] Enrolling Alice in Gold Plan...\n");
    enroll_policy("POL_123", alice_addr, "ALU_GOLD");
    
    // Force expire the policy for testing
    Policy *p = get_policy("POL_123");
    p->expiry_date = time(NULL) - 86400; // Expired yesterday

    Transaction claim_tx;
    memset(&claim_tx, 0, sizeof(Transaction));
    strcpy(claim_tx.transaction_id, "TX_CLAIM_001");
    strcpy(claim_tx.sender_address, alice_addr);
    strcpy(claim_tx.receiver_address, provider_addr);
    claim_tx.amount = 200;
    claim_tx.transaction_type = TX_CLAIM_SUBMISSION;
    claim_tx.timestamp = time(NULL);

    printf("Submitting claim against expired policy...\n");
    submit_claim(&claim_tx, 10, alice_wallet, "POL_123");

    // Renew and try again
    printf("\nRenewing Policy...\n");
    renew_policy("POL_123");
    printf("Submitting claim against renewed policy...\n");
    if (submit_claim(&claim_tx, 10, alice_wallet, "POL_123")) {
        printf("Claim successfully added to mempool!\n");
    }

    // 2. Test Duplicate Fraud Detection
    printf("\n[Test 2] Submitting EXACT same transaction ID to trigger Duplicate Fraud...\n");
    submit_claim(&claim_tx, 10, alice_wallet, "POL_123");

    // 3. Test High Frequency Fraud Detection
    printf("\n[Test 3] Spamming claims to trigger High Frequency Fraud (>3 in 24h)...\n");
    for (int i = 2; i <= 4; i++) {
        Transaction spam_tx;
        memset(&spam_tx, 0, sizeof(Transaction));
        snprintf(spam_tx.transaction_id, 64, "TX_CLAIM_00%d", i);
        strcpy(spam_tx.sender_address, alice_addr);
        strcpy(spam_tx.receiver_address, provider_addr);
        spam_tx.amount = 50;
        spam_tx.transaction_type = TX_CLAIM_SUBMISSION;
        spam_tx.timestamp = time(NULL);
        submit_claim(&spam_tx, 5, alice_wallet, "POL_123");
    }

    // Verify Suspicious Status
    printf("\nMempool Verification: Checking for Suspicious tags...\n");
    view_mempool();

    EC_KEY_free(alice_wallet);
    return 0;
}
