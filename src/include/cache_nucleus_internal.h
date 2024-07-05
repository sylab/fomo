#ifndef CACHE_NUCLEUS_INTERNAL_H
#define CACHE_NUCLEUS_INTERNAL_H

#include "cache_nucleus_struct.h"
#include "common.h"
#include "entry.h"
#include "policy_stats.h"
#include "tools/epool.h"
#include "tools/hashtable.h"
#include "tools/queue.h"
#include "writeback_tracker.h"

typedef struct cache_nucleus *(*policy_create_f)(cblock_t cache_size,
                                                 cblock_t meta_size);

// Common internal functions

/** alloc_pool_entry:
 * Allocate an entry from the speficied entry_pool
 *
 * \param bp cache_nucleus to allocate entry for
 * \param epool entry_pool to allocate entry from
 * \param oblock origin device block address of allocated entry
 * \return Pointer to allocated entry
 * \memberof cache_nucleus
 */
static struct entry *alloc_pool_entry(struct cache_nucleus *bp,
                                      struct entry_pool *epool,
                                      oblock_t oblock) {
  struct entry *e = alloc_entry(epool);
  LOG_ASSERT(e != NULL);

  e->oblock = oblock;
  e->dirty = false;
  hash_insert(&bp->ht, e);

  return e;
}

/** alloc_pool_entry_at:
 * Allocate an entry from the specified entry_pool at a particular cblock
 *
 * \param bp cache_nucleus to allocate entry for
 * \param epool entry_pool to allocate entry for
 * \param oblock origin device block address of allocated entry
 * \param cblock cache device block address for allocated entry
 * \return Pointer to allocated entry
 * \memberof cache_nucleus
 */
static struct entry *alloc_pool_entry_at(struct cache_nucleus *bp,
                                         struct entry_pool *epool,
                                         oblock_t oblock, cblock_t cblock) {
  struct entry *e = alloc_particular_entry(epool, cblock);
  LOG_ASSERT(e != NULL);

  e->oblock = oblock;
  e->dirty = false;
  hash_insert(&bp->ht, e);

  return e;
}

/** insert_in_meta:
 * Insert entry in meta cache
 */
static struct entry *insert_in_meta(struct cache_nucleus *bp, oblock_t oblock) {
  struct entry *e = alloc_pool_entry(bp, &bp->meta_pool, oblock);
  return e;
}

/** in_meta:
 * Is entry in meta cache?
 */
static bool in_meta(struct cache_nucleus *bp, struct entry *e) {
  return in_pool(&bp->meta_pool, e);
}

/** meta_is_full:
 * Is the meta cache full?
 */
static bool meta_is_full(struct cache_nucleus *bp) {
  return epool_empty(&bp->meta_pool);
}

/** insert_in_cache:
 * Insert entry in cache
 */
static struct entry *insert_in_cache(struct cache_nucleus *bp, oblock_t oblock,
                                     struct cache_nucleus_result *result) {
  struct entry *e = alloc_pool_entry(bp, &bp->cache_pool, oblock);
  if (result) {
    result->cblock = infer_cblock(&bp->cache_pool, e);
  }
  return e;
}

/** insert_in_cache_at:
 * Insert entry in cache at particular cblock
 *
 * /note Reserves entry
 */
static struct entry *insert_in_cache_at(struct cache_nucleus *bp,
                                        oblock_t oblock, cblock_t cblock,
                                        struct cache_nucleus_result *result) {
  struct entry *e = alloc_pool_entry_at(bp, &bp->cache_pool, oblock, cblock);
  if (result) {
    result->cblock = cblock;
  }
  return e;
}

/** in_cache:
 * Is entry in cache?
 */
static bool in_cache(struct cache_nucleus *bp, struct entry *e) {
  return in_pool(&bp->cache_pool, e);
}

/** cache_is_full:
 * Is the cache full?
 */
static bool cache_is_full(struct cache_nucleus *bp) {
  return epool_empty(&bp->cache_pool);
}

/** cache_nucleus_remove:
 * Remove entry from cache_nucleus
 */
static void cache_nucleus_remove(struct cache_nucleus *bp, struct entry *e) {
  wb_remove(e);
  hash_remove(e);

  if (in_pool(&bp->cache_pool, e)) {
    free_entry(&bp->cache_pool, e);
  } else {
    free_entry(&bp->meta_pool, e);
  }
}

// cache_nucleus queue helper functions
#define queue_peek_entry(ptr, type, member)                                    \
  ({                                                                           \
    struct queue *q__ = (ptr);                                                 \
    struct queue_head *qh__ = (queue_peek(q__));                               \
    qh__ != NULL ? container_of(qh__, type, member) : NULL;                    \
  })

#define queue_pop_entry(ptr, type, member)                                     \
  ({                                                                           \
    struct queue *q__ = (ptr);                                                 \
    struct queue_head *qh__ = (queue_pop(q__));                                \
    qh__ != NULL ? container_of(qh__, type, member) : NULL;                    \
  })

#define heap_min_entry(ptr, type, member)                                      \
  ({                                                                           \
    struct heap *h__ = (ptr);                                                  \
    struct heap_head *hh__ = (heap_min(h__));                                  \
    hh__ != NULL ? container_of(hh__, type, member) : NULL;                    \
  })

#define heap_pop_entry(ptr, type, member)                                      \
  ({                                                                           \
    struct heap *h__ = (ptr);                                                  \
    struct heap_head *hh__ = (heap_min_pop(h__));                              \
    hh__ != NULL ? container_of(hh__, type, member) : NULL;                    \
  })

typedef bool (*queue_head_match_f)(struct cache_nucleus *bp,
                                   struct queue_head *qh);

static struct queue_head *
queue_find_first_match(struct queue *q, struct cache_nucleus *bp,
                       queue_head_match_f queue_head_match) {
  struct list_head *head = &q->queue;
  struct list_head *next = head->next;

  while (next != head) {
    struct queue_head *qh = container_of(next, struct queue_head, list);
    // prep to check next already in order to be safe in case queue_head_match
    // removes as it goes
    next = next->next;
    if (queue_head_match(bp, qh)) {
      return qh;
    }
  }
  return NULL;
}

static struct queue_head *
queue_find_last_match(struct queue *q, struct cache_nucleus *bp,
                      queue_head_match_f queue_head_match) {
  struct list_head *head = &q->queue;
  struct list_head *prev = head->prev;

  while (prev != head) {
    struct queue_head *qh = container_of(prev, struct queue_head, list);
    // prep to check prev already in order to be safe in case queue_head_match
    // removes as it goes
    prev = prev->prev;
    if (queue_head_match(bp, qh)) {
      return qh;
    }
  }
  return NULL;
}

// Default API functions for policy_init_func()
// NOTE: defaults are for the simplest of scenarios
//       policies that are more complex should use their own versions

/** default_cache_lookup:
 * Lookup cached entry with given oblock
 */
static struct entry *default_cache_lookup(struct cache_nucleus *bp,
                                          oblock_t oblock) {
  struct entry *e = hash_lookup(&bp->ht, oblock);
  if (in_cache(bp, e)) {
    return e;
  }
  return NULL;
}

/** default_meta_lookup:
 * Lookup meta entry with given oblock
 */
static struct entry *default_meta_lookup(struct cache_nucleus *bp,
                                         oblock_t oblock) {
  struct entry *e = hash_lookup(&bp->ht, oblock);
  if (in_meta(bp, e)) {
    return e;
  }
  return NULL;
}
/** default_cblock_lookup:
 * Lookup entry (allocated or not) with given cblock.
 */
static struct entry *default_cblock_lookup(struct cache_nucleus *bp,
                                           cblock_t cblock) {
  return epool_at(&bp->cache_pool, cblock);
}

/** default_remap:
 * The entry identified with current_oblock will now be identified with
 * new_oblock.
 */
static void default_remap(struct cache_nucleus *bp, oblock_t current_oblock,
                          oblock_t new_oblock) {
  struct hashtable *ht = &bp->ht;
  struct entry *e = hash_lookup(ht, current_oblock);
  hash_remove(e);
  e->oblock = new_oblock;
  hash_insert(ht, e);
}

/** default_get_stats:
 * Get the stats for a policy
 *
 * A policy_stats struct is passed down and filled due to how some policies
 * can be nested and may want to combine their stats together and pass the
 * total back to the caller (see mstar as an example for nesting policies)
 */
static void default_get_stats(struct cache_nucleus *bp,
                              struct policy_stats *stats) {
  stats->ios += bp->stats.ios;
  stats->hits += bp->stats.hits;
  stats->misses += bp->stats.misses;
  stats->filters += bp->stats.filters;
  stats->promotions += bp->stats.promotions;
  stats->demotions += bp->stats.demotions;
  stats->private_stat += bp->stats.private_stat;
}

/** default_set_time:
 * Set the time for a policy
 *
 * This is required so applications can use their own methods of time
 * measurements, which, for dm-cache, is ticks.
 * Algorithms can then use this time for calculations that are based on time.
 * The cache simulator has time be equivalent to number of IOs processed.
 */
static void default_set_time(struct cache_nucleus *bp, unsigned time) {
  bp->time = time;
}

/** default_residency:
 * How much of the cache is allocated/used?
 */
static cblock_t default_residency(struct cache_nucleus *bp) {
  return bp->cache_pool.nr_allocated;
}

/** default_cache_is_full:
 * Is the cache full?
 */
static bool default_cache_is_full(struct cache_nucleus *bp) {
  return cache_is_full(bp);
}

/** default_infer_cblock:
 * What is the cblock of the given entry?
 */
static cblock_t default_infer_cblock(struct cache_nucleus *bp,
                                     struct entry *e) {
  return infer_cblock(&bp->cache_pool, e);
}

// Policy init functions
static inline void cache_nucleus_init_api(
    struct cache_nucleus *bp, policy_map_f map, policy_remove_f remove,
    policy_insert_f insert, policy_migrated_f migrated,
    policy_cache_lookup_f cache_lookup, policy_meta_lookup_f meta_lookup,
    policy_cblock_lookup_f cblock_lookup, policy_remap_f remap,
    policy_destroy_f destroy, policy_get_stats_f get_stats,
    policy_set_time_f set_time, policy_residency_f residency,
    policy_cache_is_full_f cache_is_full, policy_infer_cblock_f infer_cblock,
    policy_next_victim_f next_victim) {
  bp->map = map;
  bp->remove = remove;
  bp->insert = insert;
  bp->migrated = migrated;
  bp->cache_lookup = cache_lookup;
  bp->meta_lookup = meta_lookup;
  bp->cblock_lookup = cblock_lookup;
  bp->remap = remap;
  bp->destroy = destroy;
  bp->get_stats = get_stats;
  bp->set_time = set_time;
  bp->residency = residency;
  bp->cache_is_full = cache_is_full;
  bp->infer_cblock = infer_cblock;
  bp->next_victim = next_victim;
}

/** Initialize the cache_nucleus
 *
 * This will initialize the entry_pool's, writeback_tracker, policy_stats,
 * hashtable, along with the cache_nucleus members.
 *
 * \warning cache_nucleus is not responsible for entry_pool creation, this must
 * be done by the algorithm policy
 *
 * \param bp cache_nucleus to initialize
 * \param cache_size Size of the cache
 * \param meta_size Size of the meta-cache
 * \return 0 if no errors occur, not 0 otherwise.
 * See code to look for specifics.
 * \memberof cache_nucleus
 */
static int policy_init(struct cache_nucleus *bp, cblock_t cache_size,
                       cblock_t meta_size,
                       enum cache_nucleus_thread_support thread_support) {
  int r;

  epool_init(&bp->cache_pool, 0u);
  epool_init(&bp->meta_pool, 0u);
  stats_init(&bp->stats);
  r = ht_init(&bp->ht, cache_size + meta_size);
  bp->cache_size = cache_size;
  bp->meta_size = meta_size;
  bp->thread_support = thread_support;
  bp->time = 0;

  return r;
}

/** Free any memory/struct initialized/owned by cache_nucleus
 *
 * \note policy_exit() does NOT exit epools due to epools not being
 *       created with policy_init(). See policy_init() for more information.
 */
static void policy_exit(struct cache_nucleus *bp) { hash_exit(&bp->ht); }

#endif /* CACHE_NUCLEUS_INTERNAL_H */
