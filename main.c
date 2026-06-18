#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "core.h"
#include "crypto.h"
#include "state.h"
#include "mempool.h"
#include "chain.h"
#include "insurance.h"
#include "persistence.h"

#define MAX_ARGS 10

// Global session variables
EC_KEY *session_wallet;
char session_addr[130];
char insurance_pool_addr[] = "ALU_INSURANCE_POOL_MAIN";
char reinsurance_pool_addr[] = "ALU_REINSURANCE_POOL_RESERVE";

void print_menu() {
    printf("\n=== ALU Health Insurance Blockchain ===\n");
    
    printf("\nMembership Management\n");
    printf("1. register_member\n");
    printf("2. view_member\n");
    printf("3. wallet_balance\n");

    printf("\nPolicy Operations\n");
    printf("4. enroll_policy\n");
    printf("5. view_policy\n");
    printf("6. renew_policy\n");
    printf("7. policy_status\n");

    printf("\nToken Operations\n");
    printf("8. token_transfer\n");
    printf("9. token_balance\n");

    printf("\nInsurance Operations\n");
    printf("10. pay_premium\n");
    printf("11. service_request\n");
    printf("12. preauth_request\n");
    printf("13. preauth_approve\n");
    printf("14. submit_claim\n");
    printf("15. approve_claim\n");
    printf("16. reject_claim\n");
    printf("17. settle_claim\n");
    printf("18. reinsurance_balance\n");

    printf("\nBlockchain Operations\n");
    printf("19. create_transaction\n");
    printf("20. mempool_view\n");
    printf("21. mine_solo\n");
    printf("22. mine_pool\n");
    printf("23. blockchain_view\n");
    printf("24. blockchain_verify\n");
    printf("25. chain_save\n");
    printf("26. chain_load\n");
    printf("27. difficulty_status\n");

    printf("\nUTXO Operations\n");
    printf("28. utxo_view\n");
    printf("29. utxo_validate\n");

    printf("\nAccount Model Operations\n");
    printf("30. account_balance\n");
    printf("31. account_transfer\n");
    printf("32. account_nonce\n");

    printf("\nFraud and Audit Operations\n");
    printf("33. fraud_review\n");
    printf("34. approve_suspicious <tx_id>\n");
    printf("35. reject_suspicious <tx_id>\n");
    printf("36. transaction_history\n");
    printf("37. provider_history\n");
    printf("38. premium_history\n");
    printf("39. claim_history\n");
    
    printf("\n0. exit\n");
    printf("=======================================\n");
}

int main() {
    printf("Booting ALU Health Insurance Blockchain...\n");
    
    if (!load_chain()) {
        printf("Starting with empty chain state.\n");
    }

    session_wallet = generate_wallet_keypair();
    get_wallet_address(session_wallet, session_addr);
    update_account_balance(session_addr, 10000, true);

    printf("\n[Session Active] Wallet Address: %s\n", session_addr);

    char input[512];
    char *args[MAX_ARGS];

    while (1) {
        print_menu();
        printf("\nALU-Chain> ");
        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "\n")] = 0; // Strip newline

        int argc = 0;
        char *token = strtok(input, " \t");
        while (token != NULL && argc < MAX_ARGS) {
            args[argc++] = token;
            token = strtok(NULL, " \t");
        }

        if (argc == 0) continue; 
        char *cmd = args[0];

        // System Control
        if (strcmp(cmd, "0") == 0 || strcmp(cmd, "exit") == 0) {
            save_chain();
            break;
        }

        // Active Commands Map
        if (strcmp(cmd, "3") == 0 || strcmp(cmd, "wallet_balance") == 0) {
            char *addr = (argc > 1) ? args[1] : session_addr;
            Account *acc = get_or_create_account(addr);
            printf("Balance: %lu AHT | Nonce: %lu\n", acc->balance, acc->nonce);
        }
        else if (strcmp(cmd, "4") == 0 || strcmp(cmd, "enroll_policy") == 0) {
            if (argc < 2) { printf("Usage: 4 <plan_name>\n"); continue; }
            char pol_id[32];
            snprintf(pol_id, 32, "POL_%ld", time(NULL));
            if (enroll_policy(pol_id, session_addr, args[1])) {
                printf("Enrolled %s in %s plan.\n", pol_id, args[1]);
            }
        }
        else if (strcmp(cmd, "6") == 0 || strcmp(cmd, "renew_policy") == 0) {
            if (argc < 2) { printf("Usage: 6 <policy_id>\n"); continue; }
            if (renew_policy(args[1])) printf("Policy %s renewed.\n", args[1]);
            else printf("Policy not found.\n");
        }
        else if (strcmp(cmd, "10") == 0 || strcmp(cmd, "pay_premium") == 0) {
            if (argc < 3) { printf("Usage: 10 <amount> <fee>\n"); continue; }
            if (submit_premium_payment(session_addr, insurance_pool_addr, reinsurance_pool_addr, strtoull(args[1], NULL, 10), strtoull(args[2], NULL, 10), session_wallet)) {
                printf("Premium submitted. Reinsurance split applied.\n");
            }
        }
        else if (strcmp(cmd, "14") == 0 || strcmp(cmd, "submit_claim") == 0) {
            if (argc < 4) { printf("Usage: 14 <provider_addr> <amount> <policy_id>\n"); continue; }
            Transaction tx;
            memset(&tx, 0, sizeof(Transaction));
            snprintf(tx.transaction_id, 64, "TX_CLM_%ld", time(NULL));
            strcpy(tx.sender_address, session_addr);
            strcpy(tx.receiver_address, args[1]);
            tx.amount = strtoull(args[2], NULL, 10);
            tx.transaction_type = TX_CLAIM_SUBMISSION;
            tx.timestamp = time(NULL);
            
            if (submit_claim(&tx, 10, session_wallet, args[3])) { // Default fee 10
                printf("Claim submitted to mempool.\n");
            }
        }
        else if (strcmp(cmd, "20") == 0 || strcmp(cmd, "mempool_view") == 0) {
            view_mempool();
        }
        else if (strcmp(cmd, "21") == 0 || strcmp(cmd, "mine_solo") == 0) {
            if (mine_solo(session_addr)) {
                printf("\n--- Block successfully mined! ---\n");
                save_chain();
            }
        }
        else if (strcmp(cmd, "23") == 0 || strcmp(cmd, "blockchain_view") == 0) {
            printf("\nBlocks: %u | Difficulty: %u | Reward: %lu AHT\n", chain_state.total_blocks, chain_state.current_difficulty, chain_state.block_reward);
        }
        else if (strcmp(cmd, "24") == 0 || strcmp(cmd, "blockchain_verify") == 0) {
            if (verify_chain()) printf("[SUCCESS] Chain is cryptographically sound.\n");
            else printf("[CRITICAL] Chain corruption detected!\n");
        }
        else if (strcmp(cmd, "25") == 0 || strcmp(cmd, "chain_save") == 0) {
            save_chain();
        }
        else if (strcmp(cmd, "26") == 0 || strcmp(cmd, "chain_load") == 0) {
            load_chain();
        }
        else if (strcmp(cmd, "33") == 0 || strcmp(cmd, "fraud_review") == 0) {
            printf("\n--- Suspicious Transactions Pending Review ---\n");
            for (uint32_t i = 0; i < mempool_count; i++) {
                if (mempool[i].status == STATUS_SUSPICIOUS) {
                    printf("TX ID: %s | Amount: %lu\n", mempool[i].transaction_id, mempool[i].amount);
                }
            }
        }
        else if (strcmp(cmd, "34") == 0 || strcmp(cmd, "approve_suspicious") == 0) {
            if (argc < 2) { printf("Usage: 34 <tx_id>\n"); continue; }
            for (uint32_t i = 0; i < mempool_count; i++) {
                if (strcmp(mempool[i].transaction_id, args[1]) == 0 && mempool[i].status == STATUS_SUSPICIOUS) {
                    mempool[i].status = STATUS_PENDING;
                    printf("Transaction %s approved.\n", args[1]);
                }
            }
        }
        else if (strcmp(cmd, "35") == 0 || strcmp(cmd, "reject_suspicious") == 0) {
            if (argc < 2) { printf("Usage: 35 <tx_id>\n"); continue; }
            Transaction mock; strcpy(mock.transaction_id, args[1]);
            remove_confirmed_transactions(&mock, 1);
            printf("Transaction %s permanently rejected.\n", args[1]);
        }
        else {
            printf("[System] Command recognized but backend integration pending. Please select another option for the demo.\n");
        }
    }

    EC_KEY_free(session_wallet);
    return 0;
}
