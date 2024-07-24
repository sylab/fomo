#ifndef ALG_WRAPPER_REGISTRY_H
#define ALG_WRAPPER_REGISTRY_H

#include "cache_nucleus.h"
#include "alg_wrapper.h"

typedef struct alg_wrapper *(*alg_wrapper_create_f)(cblock_t cache_size,
                                                    cblock_t meta_size);

#define POLICY_NAME_MAX_LENGTH 60

static bool __wrapper_name_match(const char *name_a, const char *name_b) {
  return strncasecmp(name_a, name_b, POLICY_NAME_MAX_LENGTH) == 0;
}

#include "algs/arc/arc_wrapper.h"
#include "algs/larc/larc_wrapper.h"
#include "algs/lfu/lfu_wrapper.h"
#include "algs/lirs/lirs_wrapper.h"
#include "algs/lru/lru_wrapper.h"
#include "algs/marc/marc_wrapper.h"
#include "mstar/mstar_wrapper.h"

#include "algs/arc/arc_policy.h"
static struct alg_wrapper *mstar_arc_wrapper_create(cblock_t cache_size, cblock_t meta_size) {
  return mstar_wrapper_create(cache_size, meta_size, arc_create, false);
}

static struct alg_wrapper *mstar_arc_lw_wrapper_create(cblock_t cache_size, cblock_t meta_size) {
  return mstar_wrapper_create(cache_size, meta_size, arc_create, true);
}

#include "algs/larc/larc_policy.h"
static struct alg_wrapper *mstar_larc_wrapper_create(cblock_t cache_size, cblock_t meta_size) {
  return mstar_wrapper_create(cache_size, meta_size, larc_create, false);
}

static struct alg_wrapper *mstar_larc_lw_wrapper_create(cblock_t cache_size, cblock_t meta_size) {
  return mstar_wrapper_create(cache_size, meta_size, larc_create, true);
}

#include "algs/lfu/lfu_policy.h"
static struct alg_wrapper *mstar_lfu_wrapper_create(cblock_t cache_size, cblock_t meta_size) {
  return mstar_wrapper_create(cache_size, meta_size, lfu_create, false);
}

static struct alg_wrapper *mstar_lfu_lw_wrapper_create(cblock_t cache_size, cblock_t meta_size) {
  return mstar_wrapper_create(cache_size, meta_size, lfu_create, true);
}

#include "algs/lirs/lirs_policy.h"
static struct alg_wrapper *mstar_lirs_wrapper_create(cblock_t cache_size, cblock_t meta_size) {
  return mstar_wrapper_create(cache_size, meta_size, lirs_create, false);
}

static struct alg_wrapper *mstar_lirs_lw_wrapper_create(cblock_t cache_size, cblock_t meta_size) {
  return mstar_wrapper_create(cache_size, meta_size, lirs_create, true);
}

#include "algs/lru/lru_policy.h"
static struct alg_wrapper *mstar_lru_wrapper_create(cblock_t cache_size, cblock_t meta_size) {
  return mstar_wrapper_create(cache_size, meta_size, lru_create, false);
}

static struct alg_wrapper *mstar_lru_lw_wrapper_create(cblock_t cache_size, cblock_t meta_size) {
  return mstar_wrapper_create(cache_size, meta_size, lru_create, true);
}

#include "algs/marc/marc_policy.h"
static struct alg_wrapper *mstar_marc_wrapper_create(cblock_t cache_size, cblock_t meta_size) {
  return mstar_wrapper_create(cache_size, meta_size, marc_create, false);
}

static struct alg_wrapper *mstar_marc_lw_wrapper_create(cblock_t cache_size, cblock_t meta_size) {
  return mstar_wrapper_create(cache_size, meta_size, marc_create, true);
}

#include "fomo/fomo_wrapper.h"

#include "algs/arc/arc_policy.h"
static struct alg_wrapper *fomo_arc_wrapper_create(cblock_t cache_size, cblock_t meta_size) {
  return fomo_wrapper_create(cache_size, meta_size, arc_create);
}

#include "algs/larc/larc_policy.h"
static struct alg_wrapper *fomo_larc_wrapper_create(cblock_t cache_size, cblock_t meta_size) {
  return fomo_wrapper_create(cache_size, meta_size, larc_create);
}

#include "algs/lfu/lfu_policy.h"
static struct alg_wrapper *fomo_lfu_wrapper_create(cblock_t cache_size, cblock_t meta_size) {
  return fomo_wrapper_create(cache_size, meta_size, lfu_create);
}

#include "algs/lirs/lirs_policy.h"
static struct alg_wrapper *fomo_lirs_wrapper_create(cblock_t cache_size, cblock_t meta_size) {
  return fomo_wrapper_create(cache_size, meta_size, lirs_create);
}

#include "algs/lru/lru_policy.h"
static struct alg_wrapper *fomo_lru_wrapper_create(cblock_t cache_size, cblock_t meta_size) {
  return fomo_wrapper_create(cache_size, meta_size, lru_create);
}

#include "algs/marc/marc_policy.h"
static struct alg_wrapper *fomo_marc_wrapper_create(cblock_t cache_size, cblock_t meta_size) {
  return fomo_wrapper_create(cache_size, meta_size, marc_create);
}

static alg_wrapper_create_f find_wrapper(const char *name) {

  if (__wrapper_name_match(name, "arc")) {
    return arc_wrapper_create;
  }
  if (__wrapper_name_match(name, "larc")) {
    return larc_wrapper_create;
  }
  if (__wrapper_name_match(name, "lfu")) {
    return lfu_wrapper_create;
  }
  if (__wrapper_name_match(name, "lirs")) {
    return lirs_wrapper_create;
  }
  if (__wrapper_name_match(name, "lru")) {
    return lru_wrapper_create;
  }
  if (__wrapper_name_match(name, "marc")) {
    return marc_wrapper_create;
  }
  if (__wrapper_name_match(name, "mstar_arc")) {
    return mstar_arc_wrapper_create;
  }
  if (__wrapper_name_match(name, "mstar_arc_lw")) {
    return mstar_arc_lw_wrapper_create;
  }
  if (__wrapper_name_match(name, "mstar_larc")) {
    return mstar_larc_wrapper_create;
  }
  if (__wrapper_name_match(name, "mstar_larc_lw")) {
    return mstar_larc_lw_wrapper_create;
  }
  if (__wrapper_name_match(name, "mstar_lfu")) {
    return mstar_lfu_wrapper_create;
  }
  if (__wrapper_name_match(name, "mstar_lfu_lw")) {
    return mstar_lfu_lw_wrapper_create;
  }
  if (__wrapper_name_match(name, "mstar_lirs")) {
    return mstar_lirs_wrapper_create;
  }
  if (__wrapper_name_match(name, "mstar_lirs_lw")) {
    return mstar_lirs_lw_wrapper_create;
  }
  if (__wrapper_name_match(name, "mstar_lru")) {
    return mstar_lru_wrapper_create;
  }
  if (__wrapper_name_match(name, "mstar_lru_lw")) {
    return mstar_lru_lw_wrapper_create;
  }
  if (__wrapper_name_match(name, "mstar_marc")) {
    return mstar_marc_wrapper_create;
  }
  if (__wrapper_name_match(name, "mstar_marc_lw")) {
    return mstar_marc_lw_wrapper_create;
  }
  if (__wrapper_name_match(name, "fomo_arc")) {
    return fomo_arc_wrapper_create;
  }
  if (__wrapper_name_match(name, "fomo_larc")) {
    return fomo_larc_wrapper_create;
  }
  if (__wrapper_name_match(name, "fomo_lfu")) {
    return fomo_lfu_wrapper_create;
  }
  if (__wrapper_name_match(name, "fomo_lirs")) {
    return fomo_lirs_wrapper_create;
  }
  if (__wrapper_name_match(name, "fomo_lru")) {
    return fomo_lru_wrapper_create;
  }
  if (__wrapper_name_match(name, "fomo_marc")) {
    return fomo_marc_wrapper_create;
  }
  return NULL;
}

static struct alg_wrapper *create_wrapper(const char *name,
                                         cblock_t cache_size,
					 cblock_t meta_size) {
  alg_wrapper_create_f create_f = find_wrapper(name);
  if (create_f) {
    return create_f(cache_size, meta_size);
  }
  return NULL;
}

#endif
