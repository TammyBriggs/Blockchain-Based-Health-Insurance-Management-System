#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include "core.h"
#include <stdbool.h>

#define DATA_FILE "chain_data.bin"

bool save_chain();
bool load_chain();
bool verify_chain();

#endif // PERSISTENCE_H
