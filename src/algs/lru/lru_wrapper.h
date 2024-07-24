#ifndef ALGS_LRU_LRU_WRAPPER_H
#define ALGS_LRU_LRU_WRAPPER_H

#include "alg_wrapper.h"

struct alg_wrapper *lru_wrapper_create(cblock_t cache_size, cblock_t meta_size);

#endif /* ALGS_LRU_LRU_WRAPPER_H */
