#ifndef ALGS_INCLUDE_MSTAR_POLICY_H
#define ALGS_INCLUDE_MSTAR_POLICY_H

#include "cache_nucleus_internal.h"

struct cache_nucleus *
mstar_create(cblock_t cache_size, cblock_t meta_size,
             struct cache_nucleus *(*policy_create)(cblock_t cache_size,
                                                    cblock_t meta_size),
             bool adaptive);

#endif /* ALGS_INCLUDE_MSTAR_POLICY_H */
