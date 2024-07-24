/* dmcache_base
 * Kernel module only implementation written for
 * cache_nucleus to interact with Linux's dm-cache
 *
 * Implements everything for dm-cache so cache_nucleus
 * requires *no knowledge* of dm-cache for it to work
 */

#ifndef DMCACHE_POLICY_DMCACHE_BASE_H
#define DMCACHE_POLICY_DMCACHE_BASE_H

#include "cache_nucleus.h"
#include "dm-cache-policy.h"
#include "dm.h"
#include "writeback_tracker.h"

#include <linux/mutex.h>
#include <linux/spinlock.h>

#define DM_MSG_PREFIX "dmcache-policy"

/** dmcache_base:
 * Includes dm_cache_policy for the dm-cache-policy API
 * Points to a cache_nucleus that it creates and operates on
 * Protects itself from multithreading with a mutex lock
 * Keeps track of time with ticks
 *  Ticks are measured by io completions by dm-cache
 * Keeps track of special stats that occur on this level
 *  lockfail - cannot get the lock when trying to access entry
 *  skipped  - cannot do any work with the cache (cache_nucleus)
 *             likely due to being unable to "migrate" (or
 *             move entries into the cache)
 * Has the option to use writeback and ticks (both are off by default)
 * Keeps track of writebacks for dm-cache
 */
struct dmcache_base {
  struct dm_cache_policy dm_cache_policy;
  struct cache_nucleus *policy;

  // stats
  struct policy_stats stats;

  // add lock
  struct mutex lock;

  // add tick stuff
  spinlock_t tick_lock;
  unsigned tick;
  unsigned tick_protected;

  // add stats
  unsigned lockfail;
  unsigned skipped;

  // add features
  bool use_wb;
  bool use_tick;

  // TODO: is this needed?
  // writeback tracker
  struct writeback_tracker wb_tracker;
};

struct dmcache_base *to_dmcache_base(struct dm_cache_policy *dm_cache_p) {
  return container_of(dm_cache_p, struct dmcache_base, dm_cache_policy);
}

static void dmcache_base_destroy(struct dm_cache_policy *dm_cache_p) {
  struct dmcache_base *dmcb = to_dmcache_base(dm_cache_p);
  struct cache_nucleus *bp = dmcb->policy;
  policy_destroy(bp);
  mem_free(dmcb);
}

static void copy_tick(struct dmcache_base *dmcb) {
  struct cache_nucleus *bp = dmcb->policy;
  unsigned long flags;
  spin_lock_irqsave(&dmcb->tick_lock, flags);
  dmcb->tick = dmcb->tick_protected;
  policy_set_time(bp, dmcb->tick);
  spin_unlock_irqrestore(&dmcb->tick_lock, flags);
}

static int dmcache_base_map(struct dm_cache_policy *dm_cache_p, oblock_t oblock,
                            bool can_block, bool can_migrate,
                            bool discarded_oblock, struct bio *bio,
                            struct policy_result *result) {
  struct dmcache_base *dmcb = to_dmcache_base(dm_cache_p);
  struct cache_nucleus *bp = dmcb->policy;
  struct cache_nucleus_result bp_result;
  struct entry *e;
  int r = 0;
  bool hit;

  // setup
  {
    if (can_block)
      mutex_lock(&dmcb->lock);
    else if (!mutex_trylock(&dmcb->lock)) {
      ++dmcb->lockfail;
      return -EWOULDBLOCK;
    }

    copy_tick(dmcb);
    bp_result.op = CACHE_NUCLEUS_FAIL;
  }

  // map work
  {
    e = policy_cache_lookup(bp, oblock);
    hit = e != NULL;

    if (hit && ((dmcb->use_tick && dmcb->tick == e->time) || e->migrating)) {
      bp_result.cblock = infer_cblock(&bp->cache_pool, e);
      bp_result.op = CACHE_NUCLEUS_HIT;
      // count the hit, since it can't be seen by the cache_nucleus policy
      inc_hits(&dmcb->stats);
    } else if (!hit && !can_migrate) {
      r = -EWOULDBLOCK;
      ++dmcb->skipped;
    } else {
      r = policy_map(bp, oblock, false, &bp_result);

      // wb_tracker work
      {
        if (bp_result.op == CACHE_NUCLEUS_HIT ||
            bp_result.op == CACHE_NUCLEUS_NEW ||
            bp_result.op == CACHE_NUCLEUS_REPLACE) {
          e = policy_cache_lookup(bp, oblock);
          if (bp_result.op == CACHE_NUCLEUS_HIT) {
            wb_remove(e);
          }
          wb_push_cache(&dmcb->wb_tracker, e);
        }
      }
    }
  }

  // convert bp_result.op to appropriate result->op
  switch (bp_result.op) {
  case CACHE_NUCLEUS_HIT:
    result->op = POLICY_HIT;
    break;
  case CACHE_NUCLEUS_FILTER:
  case CACHE_NUCLEUS_FAIL:
    result->op = POLICY_MISS;
    r = -EWOULDBLOCK;
    break;
  case CACHE_NUCLEUS_NEW:
    result->op = POLICY_NEW;
    break;
  case CACHE_NUCLEUS_REPLACE:
    result->op = POLICY_REPLACE;
    break;
  default:
    LOG_FATAL("Unknown op: %u", bp_result.op);
  }
  result->old_oblock = bp_result.old_oblock;
  result->cblock = bp_result.cblock;

  mutex_unlock(&dmcb->lock);

  return r;
}

static int dmcache_base_lookup(struct dm_cache_policy *dm_cache_p,
                               oblock_t oblock, cblock_t *cblock) {

  int r;
  struct dmcache_base *dmcb = to_dmcache_base(dm_cache_p);

  if (!mutex_trylock(&dmcb->lock))
    return -EWOULDBLOCK;

  {
    struct cache_nucleus *bp = dmcb->policy;
    struct entry *e = policy_cache_lookup(bp, oblock);
    if (e != NULL) {
      r = 0;
    } else {
      r = -ENOENT;
    }
  }

  mutex_unlock(&dmcb->lock);

  return r;
}

static void dmcache_base_set_dirty(struct dm_cache_policy *dm_cache_p,
                                   oblock_t oblock) {
  struct dmcache_base *dmcb = to_dmcache_base(dm_cache_p);

  mutex_lock(&dmcb->lock);
  {
    struct cache_nucleus *bp = dmcb->policy;
    struct entry *e = policy_cache_lookup(bp, oblock);
    LOG_ASSERT(e != NULL);
    wb_remove(e);
    e->dirty = true;
    wb_push_cache(&dmcb->wb_tracker, e);
  }
  mutex_unlock(&dmcb->lock);
}

static void dmcache_base_clear_dirty(struct dm_cache_policy *dm_cache_p,
                                     oblock_t oblock) {
  struct dmcache_base *dmcb = to_dmcache_base(dm_cache_p);

  mutex_lock(&dmcb->lock);
  {
    struct cache_nucleus *bp = dmcb->policy;
    struct entry *e = policy_cache_lookup(bp, oblock);
    LOG_ASSERT(e != NULL);
    wb_remove(e);
    e->dirty = false;
    wb_push_cache(&dmcb->wb_tracker, e);
  }
  mutex_unlock(&dmcb->lock);
}

/** Preload the cache with entries that were saved from a previous instance of
 * dm-cache, that are expected to still be in the cache disk
 */
static int dmcache_base_load_mapping(struct dm_cache_policy *dm_cache_p,
                                     oblock_t oblock, cblock_t cblock,
                                     uint32_t hint, bool hint_valid) {
  struct dmcache_base *dmcb = to_dmcache_base(dm_cache_p);
  struct cache_nucleus *bp = dmcb->policy;

  policy_insert(bp, oblock, cblock);

  return 0;
}

/** Remove an oblock from the cache (Also removed from metadata since the
 * dm-cache may be doing this for error-correction reasons)
 */
static void dmcache_base_remove_mapping(struct dm_cache_policy *dm_cache_p,
                                        oblock_t oblock) {
  struct dmcache_base *dmcb = to_dmcache_base(dm_cache_p);

  mutex_lock(&dmcb->lock);
  {
    struct cache_nucleus *bp = dmcb->policy;
    struct entry *e = policy_cache_lookup(bp, oblock);
    LOG_ASSERT(e != NULL);

    wb_remove(e);
    policy_remove(bp, oblock, false);
  }
  mutex_unlock(&dmcb->lock);
}

static int dmcache_base_remove_cblock(struct dm_cache_policy *dm_cache_p,
                                      cblock_t cblock) {
  int r;
  struct dmcache_base *dmcb = to_dmcache_base(dm_cache_p);
  struct cache_nucleus *bp = dmcb->policy;

  mutex_lock(&dmcb->lock);
  {
    struct entry *e = policy_cblock_lookup(bp, cblock);
    if (e != NULL) {
      policy_remove(bp, e->oblock, false);
      wb_remove(e);
      r = 0;
    } else {
      r = -ENODATA;
    }
  }
  mutex_unlock(&dmcb->lock);

  return r;
}

static int dmcache_base_writeback_work(struct dm_cache_policy *dm_cache_p,
                                       oblock_t *oblock, cblock_t *cblock) {
  int r;
  struct dmcache_base *dmcb = to_dmcache_base(dm_cache_p);
  struct cache_nucleus *bp = dmcb->policy;

  mutex_lock(&dmcb->lock);
  {
    struct entry *e = wb_pop_dirty(&dmcb->wb_tracker);
    if (e != NULL) {
      *cblock = policy_infer_cblock(bp, e);
      *oblock = e->oblock;
      e->dirty = false;
      wb_push_cache(&dmcb->wb_tracker, e);

      r = 0;
    } else {
      r = -ENODATA;
    }
  }
  mutex_unlock(&dmcb->lock);

  return r;
}

/** Force a mapping of an entry, which is supposed to be in cache, to that of a
 * new oblock.
 * In the case that the new oblock is in the metadata (or somehow the cache), we
 * remove the entry already associated with the new oblock before associating
 * the current_oblock's entry with new_oblock
 */
static void dmcache_base_force_mapping(struct dm_cache_policy *dm_cache_p,
                                       oblock_t current_oblock,
                                       oblock_t new_oblock) {
  struct dmcache_base *dmcb = to_dmcache_base(dm_cache_p);
  struct cache_nucleus *bp = dmcb->policy;

  mutex_lock(&dmcb->lock);
  {
    // Remove existing (conflicting) entry with oblock == new_oblock
    // in case existing new_oblock is in cache or meta
    policy_remove(bp, new_oblock, false);

    // current_oblock -> new_oblock
    {
      struct entry *mapped_e = policy_cache_lookup(bp, current_oblock);
      policy_remap(bp, current_oblock, new_oblock);
      LOG_ASSERT(mapped_e != NULL);

      wb_remove(mapped_e);
      mapped_e->dirty = true;
      wb_push_cache(&dmcb->wb_tracker, mapped_e);
    }
  }
  mutex_unlock(&dmcb->lock);
}

/** How many entries are in the cache?
 */
static cblock_t dmcache_base_residency(struct dm_cache_policy *dm_cache_p) {
  struct dmcache_base *dmcb = to_dmcache_base(dm_cache_p);
  struct cache_nucleus *bp = dmcb->policy;
  cblock_t r;

  mutex_lock(&dmcb->lock);
  { r = to_cblock(policy_residency(bp)); }
  mutex_unlock(&dmcb->lock);

  return r;
}

static void dmcache_base_tick(struct dm_cache_policy *dm_cache_p) {
  struct dmcache_base *dmcb = to_dmcache_base(dm_cache_p);
  unsigned long flags;

  spin_lock_irqsave(&dmcb->tick_lock, flags);
  ++dmcb->tick_protected;
  spin_unlock_irqrestore(&dmcb->tick_lock, flags);
}

static int dmcache_base_emit_config_values(struct dm_cache_policy *dm_cache_p,
                                           char *result, unsigned maxlen) {
  ssize_t sz = 0;
  struct dmcache_base *dmcb = to_dmcache_base(dm_cache_p);
  struct cache_nucleus *bp = dmcb->policy;
  struct policy_stats stats;
  stats_init(&stats);
  policy_get_stats(bp, &stats);

  // add the policy_stats that the dmcache_base was tracking
  // because the cache_nucleus policy couldn't
  stats.hits += dmcb->stats.hits;
  stats.demotions += dmcb->stats.demotions;

  DMEMIT("use_tick %u "
         "use_wb %u "
         "policy_stats %u %u %u %u %u %u %u %u",
         dmcb->use_tick, dmcb->use_wb, stats.ios, stats.hits, stats.misses,
         stats.filters, dmcb->lockfail, dmcb->skipped, stats.promotions,
         stats.demotions);

  return 0;
}

static int dmcache_base_set_config_value(struct dm_cache_policy *dm_cache_p,
                                         const char *key, const char *value) {
  struct dmcache_base *dmcb = to_dmcache_base(dm_cache_p);
  unsigned long tmp;

  if (kstrtoul(value, 10, &tmp))
    return -EINVAL;

  // TODO consider letting >1 and 0 dictate true/on or false/off
  if (!strcasecmp(key, "use_tick")) {
    dmcb->use_tick = true;
  } else if (!strcasecmp(key, "use_wb")) {
    dmcb->use_wb = true;
    dmcb->dm_cache_policy.writeback_work = dmcache_base_writeback_work;
  } else {
    return -EINVAL;
  }

  return 0;
}

/** Called when an IO completes to `validate` an entry as fully being in the
 * cache, ready to be evicted
 */
static void dmcache_base_io_complete(struct dm_cache_policy *dm_cache_p,
                                     oblock_t oblock) {
  struct dmcache_base *dmcb = to_dmcache_base(dm_cache_p);
  struct cache_nucleus *bp = dmcb->policy;

  mutex_lock(&dmcb->lock);
  { policy_migrated(bp, oblock); }
  mutex_unlock(&dmcb->lock);
}

typedef struct cache_nucleus *(*policy_create_f)(cblock_t cache_size,
                                                 cblock_t meta_size);

static int dmcache_base_init(struct dmcache_base *dmcb, cblock_t cache_size) {
  int r;

  // init functions for dm_cache_policy
  // OPTIONAL: walk_mappings
  //           writeback_work
  //           policy_set_config_value
  dmcb->dm_cache_policy.destroy = dmcache_base_destroy;
  dmcb->dm_cache_policy.map = dmcache_base_map;
  dmcb->dm_cache_policy.lookup = dmcache_base_lookup;
  dmcb->dm_cache_policy.set_dirty = dmcache_base_set_dirty;
  dmcb->dm_cache_policy.clear_dirty = dmcache_base_clear_dirty;
  dmcb->dm_cache_policy.load_mapping = dmcache_base_load_mapping;
  // TODO is this even possible? or reasonable for us?
  //  dmcb->dm_cache_policy.walk_mappings = dmcache_base_walk_mappings;
  dmcb->dm_cache_policy.walk_mappings = NULL;
  dmcb->dm_cache_policy.remove_mapping = dmcache_base_remove_mapping;
  dmcb->dm_cache_policy.remove_cblock = dmcache_base_remove_cblock;
  //  NOTE: off by default. turn on with use_wb
  //  dmcb->dm_cache_policy.writeback_work = dmcache_base_writeback_work;
  dmcb->dm_cache_policy.writeback_work = NULL;
  dmcb->dm_cache_policy.force_mapping = dmcache_base_force_mapping;
  dmcb->dm_cache_policy.residency = dmcache_base_residency;
  dmcb->dm_cache_policy.tick = dmcache_base_tick;
  dmcb->dm_cache_policy.emit_config_values = dmcache_base_emit_config_values;
  dmcb->dm_cache_policy.set_config_value = dmcache_base_set_config_value;

  // TODO: is this right?
  dmcb->dm_cache_policy.io_complete = dmcache_base_io_complete;

  // stats
  stats_init(&dmcb->stats);

  // lock
  mutex_init(&dmcb->lock);

  // tick stuff
  spin_lock_init(&dmcb->tick_lock);
  dmcb->tick = 0;
  dmcb->tick_protected = 0;

  // stats
  dmcb->lockfail = 0;
  dmcb->skipped = 0;

  // features
  dmcb->use_wb = false;
  dmcb->use_tick = false;

  // writeback tracker
  wb_init(&dmcb->wb_tracker);

  return 0;
}

static struct dmcache_base *dmcache_base_create(policy_create_f policy_create,
                                                cblock_t cache_size) {
  // TODO: consider keeping this kmalloc'd
  struct dmcache_base *dmcb = mem_alloc(sizeof(*dmcb));
  struct cache_nucleus *bp;
  if (!dmcb) {
    goto dmcache_base_alloc_fail;
  }

  // NOTE: default to cache_size metadata cache
  bp = policy_create(cache_size, cache_size);
  if (!bp) {
    goto cache_nucleus_create_fail;
  }
  dmcb->policy = bp;

  if (dmcache_base_init(dmcb, cache_size)) {
    goto dmcache_base_init_fail;
  }

  return dmcb;

dmcache_base_init_fail:
  bp->destroy(bp);
cache_nucleus_create_fail:
  mem_free(dmcb);
dmcache_base_alloc_fail:
  return NULL;
}

#endif
