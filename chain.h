#ifndef CHAIN_H
#define CHAIN_H

#include "core.h"
#include <stdbool.h>

#define MAX_BLOCKS 1000

extern Block blockchain[MAX_BLOCKS];
extern ChainState chain_state;
extern Token aht_token;

void chain_init();
void compute_merkle_root(Transaction *txs, uint32_t count, unsigned char *root_out);
void hash_block(const Block *b, unsigned char *hash_out);
bool check_difficulty(const unsigned char *hash, uint32_t difficulty);
void retarget_difficulty();
bool mine_solo(const char *miner_address);
bool mine_pool(const char **miner_addresses, const uint32_t *hash_contributions, uint32_t miner_count);

#endif // CHAIN_H
