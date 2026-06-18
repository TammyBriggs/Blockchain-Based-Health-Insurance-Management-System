#ifndef INSURANCE_H
#define INSURANCE_H

#include "core.h"
#include <stdbool.h>
#include <time.h>
#include <openssl/ec.h>

#define MAX_POLICIES 100

typedef enum {
    POLICY_ACTIVE,
    POLICY_EXPIRED,
    POLICY_RENEWED
} PolicyStatus;

typedef struct {
    char policy_id[64];
    char member_address[130];
    char coverage_plan[32];
    time_t enrollment_date;
    time_t expiry_date;
    PolicyStatus status;
} Policy;

// Initialization
void insurance_init();

// Policy Management
bool enroll_policy(const char *policy_id, const char *member_address, const char *plan);
bool renew_policy(const char *policy_id);
Policy* get_policy(const char *policy_id);

// Fraud Detection Heuristics
bool is_duplicate_transaction(const char *tx_id);
bool is_high_frequency_claim(const char *member_address, time_t tx_time);
bool is_abnormal_claim_amount(const char *provider_address, uint64_t amount);
bool run_fraud_heuristics(const Transaction *tx);

// Claim Submission Wrapper
bool submit_claim(Transaction *tx, uint64_t fee, EC_KEY *priv_key, const char *policy_id);

#endif // INSURANCE_H
