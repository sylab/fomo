#ifndef CACHE_NUCLEUS_STRUCT_H
#define CACHE_NUCLEUS_STRUCT_H

#include "common.h"
#include "entry.h"
#include "policy_stats.h"
#include "tools/epool.h"
#include "tools/hashtable.h"
#include "tools/queue.h"
#include "writeback_tracker.h"

/** Operation enum for policy communication
 * CACHE_NUCLEUS_HIT      Access resulted in a hit
 * CACHE_NUCLEUS_FILTER   Access resulted in a miss, entry not placed in cache
 * CACHE_NUCLEUS_NEW      Access resulted in a miss, placing the entry in cache
 *                        using free space
 * CACHE_NUCLEUS_REPLACE  Access resulted in a miss, placing the
 *                        entry in cache and evicting a previous entry
 * CACHE_NUCLEUS_FAIL     Access was unable to be processed, entry not placed in
 *                        cache due to reasons such as an inability to evict
 */
enum cache_nucleus_operation {
  CACHE_NUCLEUS_HIT,
  CACHE_NUCLEUS_FILTER,
  CACHE_NUCLEUS_NEW,
  CACHE_NUCLEUS_REPLACE,
  CACHE_NUCLEUS_FAIL,
};

/** Threading support enum for policy
 * Useful for parallel caches to know how to interact with policy
 *
 * CACHE_NUCLEUS_SINGLE_THREAD  Typically means that locks are not considered
 *                              and caller should lock before and unlock after
 *                              calls using their own lock
 * CACHE_NUCLEUS_MULTI_THREAD   Typically means that locks are internal to the
 *                              policy and thus caller need not use their own
 */
enum cache_nucleus_thread_support {
  CACHE_NUCLEUS_SINGLE_THREAD,
  CACHE_NUCLEUS_MULTI_THREAD
};

/** Access result information for cache nucleus communication
 * op              cache_nucleus_operation that occurred
 * old_oblock      oblock that was evicted (REPLACE)
 * cblock          where the op occurred (HIT, NEW, REPLACE)
 * dirty_eviction  eviction is dirty
 */
struct cache_nucleus_result {
  enum cache_nucleus_operation op;
  oblock_t old_oblock;
  cblock_t cblock;
  bool dirty_eviction;
};

// API functions
struct cache_nucleus;

typedef int (*policy_map_f)(struct cache_nucleus *bp, oblock_t oblock,
                            bool write, struct cache_nucleus_result *result);
typedef void (*policy_remove_f)(struct cache_nucleus *bp, oblock_t oblock,
                                bool remove_to_history);
typedef void (*policy_insert_f)(struct cache_nucleus *bp, oblock_t oblock,
                                cblock_t cblock);
typedef void (*policy_migrated_f)(struct cache_nucleus *bp, oblock_t oblock);
typedef struct entry *(*policy_cache_lookup_f)(struct cache_nucleus *bp,
                                               oblock_t oblock);
typedef struct entry *(*policy_meta_lookup_f)(struct cache_nucleus *bp,
                                              oblock_t oblock);
typedef struct entry *(*policy_cblock_lookup_f)(struct cache_nucleus *bp,
                                                cblock_t cblock);
typedef void (*policy_remap_f)(struct cache_nucleus *bp,
                               oblock_t current_oblock, oblock_t new_oblock);
typedef void (*policy_destroy_f)(struct cache_nucleus *bp);
typedef void (*policy_get_stats_f)(struct cache_nucleus *bp,
                                   struct policy_stats *stats);
typedef void (*policy_set_time_f)(struct cache_nucleus *bp, unsigned time);
typedef unsigned (*policy_residency_f)(struct cache_nucleus *bp);
typedef bool (*policy_cache_is_full_f)(struct cache_nucleus *bp);
typedef cblock_t (*policy_infer_cblock_f)(struct cache_nucleus *bp,
                                          struct entry *e);
typedef enum cache_nucleus_operation (*policy_next_victim_f)(
    struct cache_nucleus *bp, oblock_t oblock, struct entry **next_victim);

/** Generic cache policy struct, the nucleus of the cache
 *
 * Intended to be used as a building block for cache algorithms to build upon.
 * Allows for the use of interfacing through functions that only need to use
 * what's available in the cache_nucleus using "C-style polymorphism".
 */
struct cache_nucleus {
  enum cache_nucleus_thread_support thread_support;

  cblock_t meta_size;
  cblock_t cache_size;

  struct entry_pool meta_pool;
  struct entry_pool cache_pool;

  struct policy_stats stats;

  struct hashtable ht;

  // NOTE: time is set externally
  unsigned time;

  // cache_nucleus API function pointers
  policy_map_f map;
  policy_remove_f remove;
  policy_insert_f insert;
  policy_migrated_f migrated;
  policy_cache_lookup_f cache_lookup;
  policy_meta_lookup_f meta_lookup;
  policy_cblock_lookup_f cblock_lookup;
  policy_remap_f remap;
  policy_destroy_f destroy;
  policy_get_stats_f get_stats;
  policy_set_time_f set_time;
  policy_residency_f residency;
  policy_cache_is_full_f cache_is_full;
  policy_infer_cblock_f infer_cblock;
  policy_next_victim_f next_victim;
};

#endif /* CACHE_NUCLEUS_STRUCT_H */
