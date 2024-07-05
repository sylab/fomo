#ifndef ALGS_FOMO_FOMO_WRAPPER_H
#define ALGS_FOMO_FOMO_WRAPPER_H

#include "alg_wrapper.h"
#include "cache_nucleus_internal.h"

struct alg_wrapper *fomo_wrapper_create(cblock_t cache_size, cblock_t meta_size,
                                        policy_create_f policy_create);

#endif /* ARGS_FOMO_FOMO_WRAPPER_H */
