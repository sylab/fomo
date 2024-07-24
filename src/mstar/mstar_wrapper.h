#ifndef ALGS_MSTAR_MSTAR_WRAPPER_H
#define ALGS_MSTAR_MSTAR_WRAPPER_H

#include "alg_wrapper.h"
#include "cache_nucleus_internal.h"

struct alg_wrapper *mstar_wrapper_create(cblock_t cache_size,
                                         cblock_t meta_size,
                                         policy_create_f policy_create,
                                         bool adaptive);

#endif /* ARGS_MSTAR_MSTAR_WRAPPER_H */
