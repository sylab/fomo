#ifndef ALGS_LRU_LRU_POLICY_H
#define ALGS_LRU_LRU_POLICY_H

#include "cache_nucleus.h"

struct cache_nucleus *lru_create(cblock_t cache_size, cblock_t meta_size);

#endif /* ALGS_LRU_LRU_POLICY_H */
