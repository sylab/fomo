#ifndef POLICY_REGISTRY_H
#define POLICY_REGISTRY_H

#include "cache_nucleus.h"

typedef struct cache_nucleus *(*policy_create_f)(cblock_t cache_size, cblock_t meta_size);

#define POLICY_NAME_MAX_LENGTH 60

static bool __policy_name_match(const char *name_a, const char *name_b) {
  return strncasecmp(name_a, name_b, POLICY_NAME_MAX_LENGTH) == 0;
}

#include "mstar/mstar_policy.h"

#include "algs/arc/arc_policy.h"
static struct cache_nucleus *mstar_arc_create(cblock_t cache_size, cblock_t meta_size) {
  return mstar_create(cache_size, meta_size, arc_create, false);
}

static struct cache_nucleus *mstar_arc_lw_create(cblock_t cache_size, cblock_t meta_size) {
  return mstar_create(cache_size, meta_size, arc_create, true);
}

#include "algs/larc/larc_policy.h"
static struct cache_nucleus *mstar_larc_create(cblock_t cache_size, cblock_t meta_size) {
  return mstar_create(cache_size, meta_size, larc_create, false);
}

static struct cache_nucleus *mstar_larc_lw_create(cblock_t cache_size, cblock_t meta_size) {
  return mstar_create(cache_size, meta_size, larc_create, true);
}

#include "algs/lfu/lfu_policy.h"
static struct cache_nucleus *mstar_lfu_create(cblock_t cache_size, cblock_t meta_size) {
  return mstar_create(cache_size, meta_size, lfu_create, false);
}

static struct cache_nucleus *mstar_lfu_lw_create(cblock_t cache_size, cblock_t meta_size) {
  return mstar_create(cache_size, meta_size, lfu_create, true);
}

#include "algs/lirs/lirs_policy.h"
static struct cache_nucleus *mstar_lirs_create(cblock_t cache_size, cblock_t meta_size) {
  return mstar_create(cache_size, meta_size, lirs_create, false);
}

static struct cache_nucleus *mstar_lirs_lw_create(cblock_t cache_size, cblock_t meta_size) {
  return mstar_create(cache_size, meta_size, lirs_create, true);
}

#include "algs/lru/lru_policy.h"
static struct cache_nucleus *mstar_lru_create(cblock_t cache_size, cblock_t meta_size) {
  return mstar_create(cache_size, meta_size, lru_create, false);
}

static struct cache_nucleus *mstar_lru_lw_create(cblock_t cache_size, cblock_t meta_size) {
  return mstar_create(cache_size, meta_size, lru_create, true);
}

#include "algs/marc/marc_policy.h"
static struct cache_nucleus *mstar_marc_create(cblock_t cache_size, cblock_t meta_size) {
  return mstar_create(cache_size, meta_size, marc_create, false);
}

static struct cache_nucleus *mstar_marc_lw_create(cblock_t cache_size, cblock_t meta_size) {
  return mstar_create(cache_size, meta_size, marc_create, true);
}

#include "fomo/fomo_policy.h"

#include "algs/arc/arc_policy.h"
static struct cache_nucleus *fomo_arc_create(cblock_t cache_size, cblock_t meta_size) {
  return fomo_create(cache_size, meta_size, arc_create);
}

#include "algs/larc/larc_policy.h"
static struct cache_nucleus *fomo_larc_create(cblock_t cache_size, cblock_t meta_size) {
  return fomo_create(cache_size, meta_size, larc_create);
}

#include "algs/lfu/lfu_policy.h"
static struct cache_nucleus *fomo_lfu_create(cblock_t cache_size, cblock_t meta_size) {
  return fomo_create(cache_size, meta_size, lfu_create);
}

#include "algs/lirs/lirs_policy.h"
static struct cache_nucleus *fomo_lirs_create(cblock_t cache_size, cblock_t meta_size) {
  return fomo_create(cache_size, meta_size, lirs_create);
}

#include "algs/lru/lru_policy.h"
static struct cache_nucleus *fomo_lru_create(cblock_t cache_size, cblock_t meta_size) {
  return fomo_create(cache_size, meta_size, lru_create);
}

#include "algs/marc/marc_policy.h"
static struct cache_nucleus *fomo_marc_create(cblock_t cache_size, cblock_t meta_size) {
  return fomo_create(cache_size, meta_size, marc_create);
}

static policy_create_f find_policy(const char *name) {

  if (__policy_name_match(name, "arc")) {
    return arc_create;
  }
  if (__policy_name_match(name, "larc")) {
    return larc_create;
  }
  if (__policy_name_match(name, "lfu")) {
    return lfu_create;
  }
  if (__policy_name_match(name, "lirs")) {
    return lirs_create;
  }
  if (__policy_name_match(name, "lru")) {
    return lru_create;
  }
  if (__policy_name_match(name, "marc")) {
    return marc_create;
  }
  if (__policy_name_match(name, "mstar_arc")) {
    return mstar_arc_create;
  }
  if (__policy_name_match(name, "mstar_arc_lw")) {
    return mstar_arc_lw_create;
  }
  if (__policy_name_match(name, "mstar_larc")) {
    return mstar_larc_create;
  }
  if (__policy_name_match(name, "mstar_larc_lw")) {
    return mstar_larc_lw_create;
  }
  if (__policy_name_match(name, "mstar_lfu")) {
    return mstar_lfu_create;
  }
  if (__policy_name_match(name, "mstar_lfu_lw")) {
    return mstar_lfu_lw_create;
  }
  if (__policy_name_match(name, "mstar_lirs")) {
    return mstar_lirs_create;
  }
  if (__policy_name_match(name, "mstar_lirs_lw")) {
    return mstar_lirs_lw_create;
  }
  if (__policy_name_match(name, "mstar_lru")) {
    return mstar_lru_create;
  }
  if (__policy_name_match(name, "mstar_lru_lw")) {
    return mstar_lru_lw_create;
  }
  if (__policy_name_match(name, "mstar_marc")) {
    return mstar_marc_create;
  }
  if (__policy_name_match(name, "mstar_marc_lw")) {
    return mstar_marc_lw_create;
  }
  if (__policy_name_match(name, "fomo_arc")) {
    return fomo_arc_create;
  }
  if (__policy_name_match(name, "fomo_larc")) {
    return fomo_larc_create;
  }
  if (__policy_name_match(name, "fomo_lfu")) {
    return fomo_lfu_create;
  }
  if (__policy_name_match(name, "fomo_lirs")) {
    return fomo_lirs_create;
  }
  if (__policy_name_match(name, "fomo_lru")) {
    return fomo_lru_create;
  }
  if (__policy_name_match(name, "fomo_marc")) {
    return fomo_marc_create;
  }
  return NULL;
}

static struct cache_nucleus *create_policy(const char *name,
                                         cblock_t cache_size,
					 cblock_t meta_size) {
  policy_create_f create_f = find_policy(name);
  if (create_f) {
    return create_f(cache_size, meta_size);
  }
  return NULL;
}

#endif
