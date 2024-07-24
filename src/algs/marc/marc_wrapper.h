#ifndef ALGS_MARC_MARC_WRAPPER_H
#define ALGS_MARC_MARC_WRAPPER_H

#include "alg_wrapper.h"

struct alg_wrapper *marc_wrapper_create(cblock_t cache_size,
                                        cblock_t meta_size);

#endif /* ALGS_MARC_MARC_WRAPPER_H */
