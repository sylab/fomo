#ifndef ALGS_LARC_LARC_WRAPPER_H
#define ALGS_LARC_LARC_WRAPPER_H

#include "alg_wrapper.h"

struct alg_wrapper *larc_wrapper_create(cblock_t cache_size,
                                        cblock_t meta_size);

#endif /* ALGS_LARC_LARC_WRAPPER_H */
