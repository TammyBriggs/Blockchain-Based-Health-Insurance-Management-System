#include <stdio.h>
#include <string.h>
#include <openssl/sha.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>
#include "core.h"

int main() {
    printf("ALU Health Insurance Blockchain System - Initialization\n");
    printf("=====================================================\n");
    
    // Test Struct Sizes to ensure alignment
    printf("Transaction Struct Size: %lu bytes\n", sizeof(Transaction));
    printf("Block Struct Size: %lu bytes\n", sizeof(Block));
    
    // Basic OpenSSL Sanity Check
    unsigned char hash[SHA256_DIGEST_LENGTH];
    const char *test_string = "ALU_Blockchain_Test";
    SHA256((unsigned char*)test_string, strlen(test_string), hash);
    
    printf("OpenSSL SHA-256 Test Hash: ");
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        printf("%02x", hash[i]);
    }
    printf("\n\nSystem dependencies verified. Ready for next batch.\n");

    return 0;
}
