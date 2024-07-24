#ifndef DMCACHE_POLICY_REGISTRY_H
#define DMCACHE_POLICY_REGISTRY_H

// TODO proper includes
#include "dmcache_policy/dmcache_base.h"

#include "algs/arc/arc_policy.h"
struct dm_cache_policy *dm_cache_policy_arc_create(cblock_t cache_size,
                                                      sector_t origin_size,
                                                      sector_t cache_block_size) {
  struct dmcache_base *dmcb = dmcache_base_create(arc_create, cache_size);
  if (!dmcb)
    return NULL;
  return &dmcb->dm_cache_policy;
}

struct dm_cache_policy_type arc_policy_type = {
  .name = "arc",
  .version = {1, 0, 0},
  .owner = THIS_MODULE,
  .create = dm_cache_policy_arc_create
};

#include "algs/larc/larc_policy.h"
struct dm_cache_policy *dm_cache_policy_larc_create(cblock_t cache_size,
                                                      sector_t origin_size,
                                                      sector_t cache_block_size) {
  struct dmcache_base *dmcb = dmcache_base_create(larc_create, cache_size);
  if (!dmcb)
    return NULL;
  return &dmcb->dm_cache_policy;
}

struct dm_cache_policy_type larc_policy_type = {
  .name = "larc",
  .version = {1, 0, 0},
  .owner = THIS_MODULE,
  .create = dm_cache_policy_larc_create
};

#include "algs/lfu/lfu_policy.h"
struct dm_cache_policy *dm_cache_policy_lfu_create(cblock_t cache_size,
                                                      sector_t origin_size,
                                                      sector_t cache_block_size) {
  struct dmcache_base *dmcb = dmcache_base_create(lfu_create, cache_size);
  if (!dmcb)
    return NULL;
  return &dmcb->dm_cache_policy;
}

struct dm_cache_policy_type lfu_policy_type = {
  .name = "lfu",
  .version = {1, 0, 0},
  .owner = THIS_MODULE,
  .create = dm_cache_policy_lfu_create
};

#include "algs/lirs/lirs_policy.h"
struct dm_cache_policy *dm_cache_policy_lirs_create(cblock_t cache_size,
                                                      sector_t origin_size,
                                                      sector_t cache_block_size) {
  struct dmcache_base *dmcb = dmcache_base_create(lirs_create, cache_size);
  if (!dmcb)
    return NULL;
  return &dmcb->dm_cache_policy;
}

struct dm_cache_policy_type lirs_policy_type = {
  .name = "lirs",
  .version = {1, 0, 0},
  .owner = THIS_MODULE,
  .create = dm_cache_policy_lirs_create
};

#include "algs/lru/lru_policy.h"
struct dm_cache_policy *dm_cache_policy_lru_create(cblock_t cache_size,
                                                      sector_t origin_size,
                                                      sector_t cache_block_size) {
  struct dmcache_base *dmcb = dmcache_base_create(lru_create, cache_size);
  if (!dmcb)
    return NULL;
  return &dmcb->dm_cache_policy;
}

struct dm_cache_policy_type lru_policy_type = {
  .name = "lru",
  .version = {1, 0, 0},
  .owner = THIS_MODULE,
  .create = dm_cache_policy_lru_create
};

#include "algs/marc/marc_policy.h"
struct dm_cache_policy *dm_cache_policy_marc_create(cblock_t cache_size,
                                                      sector_t origin_size,
                                                      sector_t cache_block_size) {
  struct dmcache_base *dmcb = dmcache_base_create(marc_create, cache_size);
  if (!dmcb)
    return NULL;
  return &dmcb->dm_cache_policy;
}

struct dm_cache_policy_type marc_policy_type = {
  .name = "marc",
  .version = {1, 0, 0},
  .owner = THIS_MODULE,
  .create = dm_cache_policy_marc_create
};

#include "mstar/mstar_policy.h"

struct cache_nucleus *mstar_arc_create(cblock_t cache_size, cblock_t meta_size) {
  return mstar_create(cache_size, meta_size, arc_create, false);
}

struct dm_cache_policy *dm_cache_policy_mstar_arc_create(cblock_t cache_size,
                                                       sector_t origin_size,
                                                       sector_t cache_block_size) {
  struct dmcache_base *dmcb = dmcache_base_create(mstar_arc_create, cache_size);
  if (!dmcb)
    return NULL;
  return &dmcb->dm_cache_policy;
}

struct dm_cache_policy_type mstar_arc_policy_type = {
  .name = "mstar_arc",
  .version = {1, 0, 0},
  .owner = THIS_MODULE,
  .create = dm_cache_policy_mstar_arc_create
};

struct cache_nucleus *mstar_arc_lw_create(cblock_t cache_size, cblock_t meta_size) {
  return mstar_create(cache_size, meta_size, arc_create, true);
}

struct dm_cache_policy *dm_cache_policy_mstar_arc_lw_create(cblock_t cache_size,
                                                          sector_t origin_size,
                                                          sector_t cache_block_size) {
  struct dmcache_base *dmcb = dmcache_base_create(mstar_arc_lw_create, cache_size);
  if (!dmcb)
    return NULL;
  return &dmcb->dm_cache_policy;
}

struct dm_cache_policy_type mstar_arc_lw_policy_type = {
  .name = "mstar_arc_lw",
  .version = {1, 0, 0},
  .owner = THIS_MODULE,
  .create = dm_cache_policy_mstar_arc_lw_create
};

struct cache_nucleus *mstar_larc_create(cblock_t cache_size, cblock_t meta_size) {
  return mstar_create(cache_size, meta_size, larc_create, false);
}

struct dm_cache_policy *dm_cache_policy_mstar_larc_create(cblock_t cache_size,
                                                       sector_t origin_size,
                                                       sector_t cache_block_size) {
  struct dmcache_base *dmcb = dmcache_base_create(mstar_larc_create, cache_size);
  if (!dmcb)
    return NULL;
  return &dmcb->dm_cache_policy;
}

struct dm_cache_policy_type mstar_larc_policy_type = {
  .name = "mstar_larc",
  .version = {1, 0, 0},
  .owner = THIS_MODULE,
  .create = dm_cache_policy_mstar_larc_create
};

struct cache_nucleus *mstar_larc_lw_create(cblock_t cache_size, cblock_t meta_size) {
  return mstar_create(cache_size, meta_size, larc_create, true);
}

struct dm_cache_policy *dm_cache_policy_mstar_larc_lw_create(cblock_t cache_size,
                                                          sector_t origin_size,
                                                          sector_t cache_block_size) {
  struct dmcache_base *dmcb = dmcache_base_create(mstar_larc_lw_create, cache_size);
  if (!dmcb)
    return NULL;
  return &dmcb->dm_cache_policy;
}

struct dm_cache_policy_type mstar_larc_lw_policy_type = {
  .name = "mstar_larc_lw",
  .version = {1, 0, 0},
  .owner = THIS_MODULE,
  .create = dm_cache_policy_mstar_larc_lw_create
};

struct cache_nucleus *mstar_lfu_create(cblock_t cache_size, cblock_t meta_size) {
  return mstar_create(cache_size, meta_size, lfu_create, false);
}

struct dm_cache_policy *dm_cache_policy_mstar_lfu_create(cblock_t cache_size,
                                                       sector_t origin_size,
                                                       sector_t cache_block_size) {
  struct dmcache_base *dmcb = dmcache_base_create(mstar_lfu_create, cache_size);
  if (!dmcb)
    return NULL;
  return &dmcb->dm_cache_policy;
}

struct dm_cache_policy_type mstar_lfu_policy_type = {
  .name = "mstar_lfu",
  .version = {1, 0, 0},
  .owner = THIS_MODULE,
  .create = dm_cache_policy_mstar_lfu_create
};

struct cache_nucleus *mstar_lfu_lw_create(cblock_t cache_size, cblock_t meta_size) {
  return mstar_create(cache_size, meta_size, lfu_create, true);
}

struct dm_cache_policy *dm_cache_policy_mstar_lfu_lw_create(cblock_t cache_size,
                                                          sector_t origin_size,
                                                          sector_t cache_block_size) {
  struct dmcache_base *dmcb = dmcache_base_create(mstar_lfu_lw_create, cache_size);
  if (!dmcb)
    return NULL;
  return &dmcb->dm_cache_policy;
}

struct dm_cache_policy_type mstar_lfu_lw_policy_type = {
  .name = "mstar_lfu_lw",
  .version = {1, 0, 0},
  .owner = THIS_MODULE,
  .create = dm_cache_policy_mstar_lfu_lw_create
};

struct cache_nucleus *mstar_lirs_create(cblock_t cache_size, cblock_t meta_size) {
  return mstar_create(cache_size, meta_size, lirs_create, false);
}

struct dm_cache_policy *dm_cache_policy_mstar_lirs_create(cblock_t cache_size,
                                                       sector_t origin_size,
                                                       sector_t cache_block_size) {
  struct dmcache_base *dmcb = dmcache_base_create(mstar_lirs_create, cache_size);
  if (!dmcb)
    return NULL;
  return &dmcb->dm_cache_policy;
}

struct dm_cache_policy_type mstar_lirs_policy_type = {
  .name = "mstar_lirs",
  .version = {1, 0, 0},
  .owner = THIS_MODULE,
  .create = dm_cache_policy_mstar_lirs_create
};

struct cache_nucleus *mstar_lirs_lw_create(cblock_t cache_size, cblock_t meta_size) {
  return mstar_create(cache_size, meta_size, lirs_create, true);
}

struct dm_cache_policy *dm_cache_policy_mstar_lirs_lw_create(cblock_t cache_size,
                                                          sector_t origin_size,
                                                          sector_t cache_block_size) {
  struct dmcache_base *dmcb = dmcache_base_create(mstar_lirs_lw_create, cache_size);
  if (!dmcb)
    return NULL;
  return &dmcb->dm_cache_policy;
}

struct dm_cache_policy_type mstar_lirs_lw_policy_type = {
  .name = "mstar_lirs_lw",
  .version = {1, 0, 0},
  .owner = THIS_MODULE,
  .create = dm_cache_policy_mstar_lirs_lw_create
};

struct cache_nucleus *mstar_lru_create(cblock_t cache_size, cblock_t meta_size) {
  return mstar_create(cache_size, meta_size, lru_create, false);
}

struct dm_cache_policy *dm_cache_policy_mstar_lru_create(cblock_t cache_size,
                                                       sector_t origin_size,
                                                       sector_t cache_block_size) {
  struct dmcache_base *dmcb = dmcache_base_create(mstar_lru_create, cache_size);
  if (!dmcb)
    return NULL;
  return &dmcb->dm_cache_policy;
}

struct dm_cache_policy_type mstar_lru_policy_type = {
  .name = "mstar_lru",
  .version = {1, 0, 0},
  .owner = THIS_MODULE,
  .create = dm_cache_policy_mstar_lru_create
};

struct cache_nucleus *mstar_lru_lw_create(cblock_t cache_size, cblock_t meta_size) {
  return mstar_create(cache_size, meta_size, lru_create, true);
}

struct dm_cache_policy *dm_cache_policy_mstar_lru_lw_create(cblock_t cache_size,
                                                          sector_t origin_size,
                                                          sector_t cache_block_size) {
  struct dmcache_base *dmcb = dmcache_base_create(mstar_lru_lw_create, cache_size);
  if (!dmcb)
    return NULL;
  return &dmcb->dm_cache_policy;
}

struct dm_cache_policy_type mstar_lru_lw_policy_type = {
  .name = "mstar_lru_lw",
  .version = {1, 0, 0},
  .owner = THIS_MODULE,
  .create = dm_cache_policy_mstar_lru_lw_create
};

struct cache_nucleus *mstar_marc_create(cblock_t cache_size, cblock_t meta_size) {
  return mstar_create(cache_size, meta_size, marc_create, false);
}

struct dm_cache_policy *dm_cache_policy_mstar_marc_create(cblock_t cache_size,
                                                       sector_t origin_size,
                                                       sector_t cache_block_size) {
  struct dmcache_base *dmcb = dmcache_base_create(mstar_marc_create, cache_size);
  if (!dmcb)
    return NULL;
  return &dmcb->dm_cache_policy;
}

struct dm_cache_policy_type mstar_marc_policy_type = {
  .name = "mstar_marc",
  .version = {1, 0, 0},
  .owner = THIS_MODULE,
  .create = dm_cache_policy_mstar_marc_create
};

struct cache_nucleus *mstar_marc_lw_create(cblock_t cache_size, cblock_t meta_size) {
  return mstar_create(cache_size, meta_size, marc_create, true);
}

struct dm_cache_policy *dm_cache_policy_mstar_marc_lw_create(cblock_t cache_size,
                                                          sector_t origin_size,
                                                          sector_t cache_block_size) {
  struct dmcache_base *dmcb = dmcache_base_create(mstar_marc_lw_create, cache_size);
  if (!dmcb)
    return NULL;
  return &dmcb->dm_cache_policy;
}

struct dm_cache_policy_type mstar_marc_lw_policy_type = {
  .name = "mstar_marc_lw",
  .version = {1, 0, 0},
  .owner = THIS_MODULE,
  .create = dm_cache_policy_mstar_marc_lw_create
};

#include "fomo/fomo_policy.h"

struct cache_nucleus *fomo_arc_create(cblock_t cache_size, cblock_t meta_size) {
  return fomo_create(cache_size, meta_size, arc_create);
}

struct dm_cache_policy *dm_cache_policy_fomo_arc_create(cblock_t cache_size,
                                                           sector_t origin_size,
                                                           sector_t cache_block_size) {
  struct dmcache_base *dmcb = dmcache_base_create(fomo_arc_create, cache_size);
  if (!dmcb)
    return NULL;
  return &dmcb->dm_cache_policy;
}

struct dm_cache_policy_type fomo_arc_policy_type = {
  .name = "fomo_arc",
  .version = {1, 0, 0},
  .owner = THIS_MODULE,
  .create = dm_cache_policy_fomo_arc_create
};

struct cache_nucleus *fomo_larc_create(cblock_t cache_size, cblock_t meta_size) {
  return fomo_create(cache_size, meta_size, larc_create);
}

struct dm_cache_policy *dm_cache_policy_fomo_larc_create(cblock_t cache_size,
                                                           sector_t origin_size,
                                                           sector_t cache_block_size) {
  struct dmcache_base *dmcb = dmcache_base_create(fomo_larc_create, cache_size);
  if (!dmcb)
    return NULL;
  return &dmcb->dm_cache_policy;
}

struct dm_cache_policy_type fomo_larc_policy_type = {
  .name = "fomo_larc",
  .version = {1, 0, 0},
  .owner = THIS_MODULE,
  .create = dm_cache_policy_fomo_larc_create
};

struct cache_nucleus *fomo_lfu_create(cblock_t cache_size, cblock_t meta_size) {
  return fomo_create(cache_size, meta_size, lfu_create);
}

struct dm_cache_policy *dm_cache_policy_fomo_lfu_create(cblock_t cache_size,
                                                           sector_t origin_size,
                                                           sector_t cache_block_size) {
  struct dmcache_base *dmcb = dmcache_base_create(fomo_lfu_create, cache_size);
  if (!dmcb)
    return NULL;
  return &dmcb->dm_cache_policy;
}

struct dm_cache_policy_type fomo_lfu_policy_type = {
  .name = "fomo_lfu",
  .version = {1, 0, 0},
  .owner = THIS_MODULE,
  .create = dm_cache_policy_fomo_lfu_create
};

struct cache_nucleus *fomo_lirs_create(cblock_t cache_size, cblock_t meta_size) {
  return fomo_create(cache_size, meta_size, lirs_create);
}

struct dm_cache_policy *dm_cache_policy_fomo_lirs_create(cblock_t cache_size,
                                                           sector_t origin_size,
                                                           sector_t cache_block_size) {
  struct dmcache_base *dmcb = dmcache_base_create(fomo_lirs_create, cache_size);
  if (!dmcb)
    return NULL;
  return &dmcb->dm_cache_policy;
}

struct dm_cache_policy_type fomo_lirs_policy_type = {
  .name = "fomo_lirs",
  .version = {1, 0, 0},
  .owner = THIS_MODULE,
  .create = dm_cache_policy_fomo_lirs_create
};

struct cache_nucleus *fomo_lru_create(cblock_t cache_size, cblock_t meta_size) {
  return fomo_create(cache_size, meta_size, lru_create);
}

struct dm_cache_policy *dm_cache_policy_fomo_lru_create(cblock_t cache_size,
                                                           sector_t origin_size,
                                                           sector_t cache_block_size) {
  struct dmcache_base *dmcb = dmcache_base_create(fomo_lru_create, cache_size);
  if (!dmcb)
    return NULL;
  return &dmcb->dm_cache_policy;
}

struct dm_cache_policy_type fomo_lru_policy_type = {
  .name = "fomo_lru",
  .version = {1, 0, 0},
  .owner = THIS_MODULE,
  .create = dm_cache_policy_fomo_lru_create
};

struct cache_nucleus *fomo_marc_create(cblock_t cache_size, cblock_t meta_size) {
  return fomo_create(cache_size, meta_size, marc_create);
}

struct dm_cache_policy *dm_cache_policy_fomo_marc_create(cblock_t cache_size,
                                                           sector_t origin_size,
                                                           sector_t cache_block_size) {
  struct dmcache_base *dmcb = dmcache_base_create(fomo_marc_create, cache_size);
  if (!dmcb)
    return NULL;
  return &dmcb->dm_cache_policy;
}

struct dm_cache_policy_type fomo_marc_policy_type = {
  .name = "fomo_marc",
  .version = {1, 0, 0},
  .owner = THIS_MODULE,
  .create = dm_cache_policy_fomo_marc_create
};

static int policy_registry_init(void) {
  int r;

  r = dm_cache_policy_register(&arc_policy_type);
  LOG_INFO("arc registry %p %d", &arc_policy_type, r);
  if (r)
    goto arc_policy_type_register_failed;

  r = dm_cache_policy_register(&larc_policy_type);
  LOG_INFO("larc registry %p %d", &larc_policy_type, r);
  if (r)
    goto larc_policy_type_register_failed;

  r = dm_cache_policy_register(&lfu_policy_type);
  LOG_INFO("lfu registry %p %d", &lfu_policy_type, r);
  if (r)
    goto lfu_policy_type_register_failed;

  r = dm_cache_policy_register(&lirs_policy_type);
  LOG_INFO("lirs registry %p %d", &lirs_policy_type, r);
  if (r)
    goto lirs_policy_type_register_failed;

  r = dm_cache_policy_register(&lru_policy_type);
  LOG_INFO("lru registry %p %d", &lru_policy_type, r);
  if (r)
    goto lru_policy_type_register_failed;

  r = dm_cache_policy_register(&marc_policy_type);
  LOG_INFO("marc registry %p %d", &marc_policy_type, r);
  if (r)
    goto marc_policy_type_register_failed;

  r = dm_cache_policy_register(&marc_policy_type);
  LOG_INFO("marc registry %p %d", &marc_policy_type, r);
  if (r)
    goto marc_policy_type_register_failed;

  r = dm_cache_policy_register(&mlarc_policy_type);
  LOG_INFO("mlarc registry %p %d", &mlarc_policy_type, r);
  if (r)
    goto mlarc_policy_type_register_failed;

  r = dm_cache_policy_register(&mlfu_policy_type);
  LOG_INFO("mlfu registry %p %d", &mlfu_policy_type, r);
  if (r)
    goto mlfu_policy_type_register_failed;

  r = dm_cache_policy_register(&mlirs_policy_type);
  LOG_INFO("mlirs registry %p %d", &mlirs_policy_type, r);
  if (r)
    goto mlirs_policy_type_register_failed;

  r = dm_cache_policy_register(&mlru_policy_type);
  LOG_INFO("mlru registry %p %d", &mlru_policy_type, r);
  if (r)
    goto mlru_policy_type_register_failed;

  r = dm_cache_policy_register(&mmarc_policy_type);
  LOG_INFO("mmarc registry %p %d", &mmarc_policy_type, r);
  if (r)
    goto mmarc_policy_type_register_failed;

  r = dm_cache_policy_register(&marc_lw_policy_type);
  LOG_INFO("marc_lw registry %p %d", &marc_lw_policy_type, r);
  if (r)
    goto marc_lw_policy_type_register_failed;

  r = dm_cache_policy_register(&mlarc_lw_policy_type);
  LOG_INFO("mlarc_lw registry %p %d", &mlarc_lw_policy_type, r);
  if (r)
    goto mlarc_lw_policy_type_register_failed;

  r = dm_cache_policy_register(&mlfu_lw_policy_type);
  LOG_INFO("mlfu_lw registry %p %d", &mlfu_lw_policy_type, r);
  if (r)
    goto mlfu_lw_policy_type_register_failed;

  r = dm_cache_policy_register(&mlirs_lw_policy_type);
  LOG_INFO("mlirs_lw registry %p %d", &mlirs_lw_policy_type, r);
  if (r)
    goto mlirs_lw_policy_type_register_failed;

  r = dm_cache_policy_register(&mlru_lw_policy_type);
  LOG_INFO("mlru_lw registry %p %d", &mlru_lw_policy_type, r);
  if (r)
    goto mlru_lw_policy_type_register_failed;

  r = dm_cache_policy_register(&mmarc_lw_policy_type);
  LOG_INFO("mmarc_lw registry %p %d", &mmarc_lw_policy_type, r);
  if (r)
    goto mmarc_lw_policy_type_register_failed;

  r = dm_cache_policy_register(&fomo_arc_policy_type);
  LOG_INFO("fomo_arc registry %p %d", &fomo_arc_policy_type, r);
  if (r)
    goto fomo_arc_policy_type_register_failed;

  r = dm_cache_policy_register(&fomo_larc_policy_type);
  LOG_INFO("fomo_larc registry %p %d", &fomo_larc_policy_type, r);
  if (r)
    goto fomo_larc_policy_type_register_failed;

  r = dm_cache_policy_register(&fomo_lfu_policy_type);
  LOG_INFO("fomo_lfu registry %p %d", &fomo_lfu_policy_type, r);
  if (r)
    goto fomo_lfu_policy_type_register_failed;

  r = dm_cache_policy_register(&fomo_lirs_policy_type);
  LOG_INFO("fomo_lirs registry %p %d", &fomo_lirs_policy_type, r);
  if (r)
    goto fomo_lirs_policy_type_register_failed;

  r = dm_cache_policy_register(&fomo_lru_policy_type);
  LOG_INFO("fomo_lru registry %p %d", &fomo_lru_policy_type, r);
  if (r)
    goto fomo_lru_policy_type_register_failed;

  r = dm_cache_policy_register(&fomo_marc_policy_type);
  LOG_INFO("fomo_marc registry %p %d", &fomo_marc_policy_type, r);
  if (r)
    goto fomo_marc_policy_type_register_failed;

  return 0;


fomo_marc_policy_type_register_failed:
  LOG_INFO("fomo_lru unregister %p", &fomo_lru_policy_type);
  dm_cache_policy_unregister(&fomo_lru_policy_type);
fomo_lru_policy_type_register_failed:
  LOG_INFO("fomo_lirs unregister %p", &fomo_lirs_policy_type);
  dm_cache_policy_unregister(&fomo_lirs_policy_type);
fomo_lirs_policy_type_register_failed:
  LOG_INFO("fomo_lfu unregister %p", &fomo_lfu_policy_type);
  dm_cache_policy_unregister(&fomo_lfu_policy_type);
fomo_lfu_policy_type_register_failed:
  LOG_INFO("fomo_larc unregister %p", &fomo_larc_policy_type);
  dm_cache_policy_unregister(&fomo_larc_policy_type);
fomo_larc_policy_type_register_failed:
  LOG_INFO("fomo_arc unregister %p", &fomo_arc_policy_type);
  dm_cache_policy_unregister(&fomo_arc_policy_type);
fomo_arc_policy_type_register_failed:
  LOG_INFO("mmarc_lw unregister %p", &mmarc_lw_policy_type);
  dm_cache_policy_unregister(&mmarc_lw_policy_type);
mmarc_lw_policy_type_register_failed:
  LOG_INFO("mlru_lw unregister %p", &mlru_lw_policy_type);
  dm_cache_policy_unregister(&mlru_lw_policy_type);
mlru_lw_policy_type_register_failed:
  LOG_INFO("mlirs_lw unregister %p", &mlirs_lw_policy_type);
  dm_cache_policy_unregister(&mlirs_lw_policy_type);
mlirs_lw_policy_type_register_failed:
  LOG_INFO("mlfu_lw unregister %p", &mlfu_lw_policy_type);
  dm_cache_policy_unregister(&mlfu_lw_policy_type);
mlfu_lw_policy_type_register_failed:
  LOG_INFO("mlarc_lw unregister %p", &mlarc_lw_policy_type);
  dm_cache_policy_unregister(&mlarc_lw_policy_type);
mlarc_lw_policy_type_register_failed:
  LOG_INFO("marc_lw unregister %p", &marc_lw_policy_type);
  dm_cache_policy_unregister(&marc_lw_policy_type);
marc_lw_policy_type_register_failed:
  LOG_INFO("mmarc unregister %p", &mmarc_policy_type);
  dm_cache_policy_unregister(&mmarc_policy_type);
mmarc_policy_type_register_failed:
  LOG_INFO("mlru unregister %p", &mlru_policy_type);
  dm_cache_policy_unregister(&mlru_policy_type);
mlru_policy_type_register_failed:
  LOG_INFO("mlirs unregister %p", &mlirs_policy_type);
  dm_cache_policy_unregister(&mlirs_policy_type);
mlirs_policy_type_register_failed:
  LOG_INFO("mlfu unregister %p", &mlfu_policy_type);
  dm_cache_policy_unregister(&mlfu_policy_type);
mlfu_policy_type_register_failed:
  LOG_INFO("mlarc unregister %p", &mlarc_policy_type);
  dm_cache_policy_unregister(&mlarc_policy_type);
mlarc_policy_type_register_failed:
  LOG_INFO("marc unregister %p", &marc_policy_type);
  dm_cache_policy_unregister(&marc_policy_type);
marc_policy_type_register_failed:
  LOG_INFO("marc unregister %p", &marc_policy_type);
  dm_cache_policy_unregister(&marc_policy_type);
marc_policy_type_register_failed:
  LOG_INFO("lru unregister %p", &lru_policy_type);
  dm_cache_policy_unregister(&lru_policy_type);
lru_policy_type_register_failed:
  LOG_INFO("lirs unregister %p", &lirs_policy_type);
  dm_cache_policy_unregister(&lirs_policy_type);
lirs_policy_type_register_failed:
  LOG_INFO("lfu unregister %p", &lfu_policy_type);
  dm_cache_policy_unregister(&lfu_policy_type);
lfu_policy_type_register_failed:
  LOG_INFO("larc unregister %p", &larc_policy_type);
  dm_cache_policy_unregister(&larc_policy_type);
larc_policy_type_register_failed:
  LOG_INFO("arc unregister %p", &arc_policy_type);
  dm_cache_policy_unregister(&arc_policy_type);
arc_policy_type_register_failed:
  return r;
}
static void policy_registry_exit(void) {
  LOG_INFO("arc unregister %p", &arc_policy_type);
  dm_cache_policy_unregister(&arc_policy_type);
  LOG_INFO("larc unregister %p", &larc_policy_type);
  dm_cache_policy_unregister(&larc_policy_type);
  LOG_INFO("lfu unregister %p", &lfu_policy_type);
  dm_cache_policy_unregister(&lfu_policy_type);
  LOG_INFO("lirs unregister %p", &lirs_policy_type);
  dm_cache_policy_unregister(&lirs_policy_type);
  LOG_INFO("lru unregister %p", &lru_policy_type);
  dm_cache_policy_unregister(&lru_policy_type);
  LOG_INFO("marc unregister %p", &marc_policy_type);
  dm_cache_policy_unregister(&marc_policy_type);
  LOG_INFO("marc unregister %p", &marc_policy_type);
  dm_cache_policy_unregister(&marc_policy_type);
  LOG_INFO("mlarc unregister %p", &mlarc_policy_type);
  dm_cache_policy_unregister(&mlarc_policy_type);
  LOG_INFO("mlfu unregister %p", &mlfu_policy_type);
  dm_cache_policy_unregister(&mlfu_policy_type);
  LOG_INFO("mlirs unregister %p", &mlirs_policy_type);
  dm_cache_policy_unregister(&mlirs_policy_type);
  LOG_INFO("mlru unregister %p", &mlru_policy_type);
  dm_cache_policy_unregister(&mlru_policy_type);
  LOG_INFO("mmarc unregister %p", &mmarc_policy_type);
  dm_cache_policy_unregister(&mmarc_policy_type);
  LOG_INFO("marc_lw unregister %p", &marc_lw_policy_type);
  dm_cache_policy_unregister(&marc_lw_policy_type);
  LOG_INFO("mlarc_lw unregister %p", &mlarc_lw_policy_type);
  dm_cache_policy_unregister(&mlarc_lw_policy_type);
  LOG_INFO("mlfu_lw unregister %p", &mlfu_lw_policy_type);
  dm_cache_policy_unregister(&mlfu_lw_policy_type);
  LOG_INFO("mlirs_lw unregister %p", &mlirs_lw_policy_type);
  dm_cache_policy_unregister(&mlirs_lw_policy_type);
  LOG_INFO("mlru_lw unregister %p", &mlru_lw_policy_type);
  dm_cache_policy_unregister(&mlru_lw_policy_type);
  LOG_INFO("mmarc_lw unregister %p", &mmarc_lw_policy_type);
  dm_cache_policy_unregister(&mmarc_lw_policy_type);
  LOG_INFO("fomo_arc unregister %p", &fomo_arc_policy_type);
  dm_cache_policy_unregister(&fomo_arc_policy_type);
  LOG_INFO("fomo_larc unregister %p", &fomo_larc_policy_type);
  dm_cache_policy_unregister(&fomo_larc_policy_type);
  LOG_INFO("fomo_lfu unregister %p", &fomo_lfu_policy_type);
  dm_cache_policy_unregister(&fomo_lfu_policy_type);
  LOG_INFO("fomo_lirs unregister %p", &fomo_lirs_policy_type);
  dm_cache_policy_unregister(&fomo_lirs_policy_type);
  LOG_INFO("fomo_lru unregister %p", &fomo_lru_policy_type);
  dm_cache_policy_unregister(&fomo_lru_policy_type);
  LOG_INFO("fomo_marc unregister %p", &fomo_marc_policy_type);
  dm_cache_policy_unregister(&fomo_marc_policy_type);
}

#endif
