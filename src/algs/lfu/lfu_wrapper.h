#ifndef ALGS_LFU_LFU_WRAPPER_H
#define ALGS_LFU_LFU_WRAPPER_H

#include "alg_wrapper.h"

struct alg_wrapper *lfu_wrapper_create(cblock_t cache_size, cblock_t meta_size);

#endif /* ARGS_LFU_LFU_WRAPPER_H */
