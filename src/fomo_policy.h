#ifndef SRC_FOMO_FOMO_POLICY_H
#define SRC_FOMO_FOMO_POLICY_H

#include "cache_nucleus_internal.h"

struct cache_nucleus *fomo_create(cblock_t cache_size, cblock_t meta_size,
                                  policy_create_f policy_create);

#endif
