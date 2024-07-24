#ifndef ALGS_LFU_LFU_POLICY_H
#define ALGS_LFU_LFU_POLICY_H

#include "cache_nucleus.h"

struct cache_nucleus *lfu_create(cblock_t cache_size, cblock_t meta_size);

#endif /* ALGS_LFU_LFU_POLICY_H */
