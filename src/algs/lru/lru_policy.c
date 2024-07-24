/* LRU caching policy
 * Implementation of the LRU caching policy using cache_nucleus.
 * As LRU is simple and can operate as an example for how to use
 * cache_nucleus, extra documentation can be found here that may
 * not be found in other, similar policies.
 *
 * Least Recently Used (LRU) Paper:
 *
 */

/** 1.0 TABLE OF CONTENTS
 *
 * 1.0     TABLE OF CONTENTS
 * 2.0     #includes
 * 3.0     The Transaction Model
 * 3.1       lru_transaction
 * 3.2       lru_transaction_init()
 * 3.3       lru_transaction_set()
 * 3.4       lru_transaction_is_legal()
 * 3.5       lru_transaction_commit()
 * 3.5.1       lru_remove_entry()
 * 3.5.2       lru_evict()
 * 3.5.3       lru_insert()
 * 3.5.4       lru_hit()
 * 3.5.5       lru_transaction_commit()
 * 4.0     Non-Default Cache Nucleus API Functions
 * 4.1       lru_remove_api()
 * 4.2       lru_insert_api()
 * 4.3       lru_migrated_api()
 * 4.4       lru_map_api()
 * 4.5       lru_destroy_api()
 * 4.6       lru_next_victim_api()
 * 5.0     LRU Creation Functions
 * 5.1       lru_init()
 * 5.2       lru_create()
 */

/** 2.0 #includes
 *
 * For the includes, we include the header that only has the
 * create function (in this case lru_create) or the lru_policy and lru_entry
 * structs, along with other important headers that have tools or structs that
 * we'll be using.
 *
 * lru_policy.h:
 *   Contains the lru_create function for external applications to use
 *
 * cache_nucleus_internal.h:
 *   Contains many functions aimed at working with the cache_nucleus struct
 *   requiring lower level interactions with cache_nucleus.
 *
 * common.h:
 *   An aggregation of includes. Includes commonly needed types and functions
 *   including: memory management, types, and logging headers.
 *
 * lru_policy_struct.h:
 *   Contains the lru_policy and lru_entry structs, primarily for internal use.
 *   NOTE: Currently also used by the lru_wrapper as well for information
 *         gathering.
 *
 * tools/queue.h:
 *   A circular, doubly-linked list for a queue.
 */
#include "lru_policy.h"
#include "cache_nucleus_internal.h"
#include "common.h"
#include "lru_policy_struct.h"
#include "tools/queue.h"

/** 3.0 The Transaction Model
 *
 * Currently a method of design for single-threaded synchronous caching
 * algorithms, like LRU, that allows them to work in asynchronous caching
 * environments with minimal changes to the original algorithm.
 *
 * Attempts to adhere closely to the principles of ACID:
 *  - Atomicity
 *      Should a part of the transaction fail, the whole transaction fails
 *  - Consistency
 *      Transactions have the algorithm go from one valid state to another
 *  - Isolation
 *      Still single-threaded, always sequential even in concurrent environment
 *  - Durability
 *      Changes should be durable after transaction is committed
 *
 * How this is achieved is through a transaction struct and few functions:
 *  - struct transaction
 *      Where changes are saved/staged
 *  - transaction_init()
 *      Where default behaviors are set
 *  - transaction_set()
 *      Where the changes are set in the transaction
 *  - transaction_is_legal()
 *      Where the changes in the transaction are checked for legality, should
 *      they be legal the changes that have been saved/staged will be committed,
 *      otherwise the transaction will fail
 *  - transaction_commit()
 *      Where the changes are finally committed to the algorithm's structures
 */

/** 3.1 lru_transaction
 *
 * A simple transaction for LRU as an example.
 *
 * Due to LRU being simple, we only need a little information.
 *
 * oblock   address being accessed
 * evict    evict LRU entry from LRU queue
 * hit      oblock is in LRU
 */
struct lru_transaction {
  oblock_t oblock;
  bool evict;
  bool hit;
};

/** 3.2 lru_transaction_init()
 *
 * A simple default initialization of the lru_transaction.
 *
 * Assumes by default:
 *  1. Not evicting
 *  2. Cache miss
 */
void lru_transaction_init(struct lru_policy *lru, struct lru_transaction *lt,
                          oblock_t oblock) {
  lt->oblock = oblock;
  lt->evict = false;
  lt->hit = false;
}

/** 3.3 lru_transaciton_set()
 *
 * Set op in the lru_transaction based on LRU's logic.
 */
void lru_transaction_set(struct lru_policy *lru, struct lru_transaction *lt) {
  struct cache_nucleus *cn = &lru->nucleus;
  if (hash_lookup(&cn->ht, lt->oblock) != NULL) {
    lt->hit = true;
  } else {
    lt->evict = cache_is_full(cn);
  }
}

/** 3.4 lru_transaction_is_legal()
 *
 * Determine if the lru_transaction's legality.
 *
 * Within an asynchronous cache we are dealing with migrating entries, so
 * theoretically all of the entries that would otherwise be in LRU's queue (and
 * therefore the cache) could be migrating.
 *
 * The only time this really matters is if we are attempting to evict from an
 * entry that has *NOT* yet been written to the cache (or whose migration has
 * not yet completed).
 *
 * NOTE: While an entry is migrating, it is not added to LRU's queue to make
 * sure that all evictions are of valid (or migrated) entries.
 *
 * Therefore, all that we check for here is that if we are evicting, then there
 * must be a fully migrated entry in LRU's queue that can be evicted.
 */
bool lru_transaction_is_legal(struct lru_policy *lru,
                              struct lru_transaction *lt) {
  if (lt->evict) {
    return queue_length(&lru->lru_q) > 0;
  }
  return true;
}

/** 3.5 lru_transaction_commit()
 *
 * Commit the changes found within the lru_transaction.
 *
 * First, we will be defining some helpful functions for work.
 */

/** 3.5.1 lru_remove_entry()
 *
 * LRU removes the given entry from itself and frees space up for a new entry.
 *
 * NOTE: As this function removes the entry from the cache, it also is
 *       responsible for incrementing the demotions count.
 *       However, entries that are removed but have not completed migrating do
 *       not count towards demotions as they weren't yet in the cache in the
 *       first place. It is assumed that the migrated entry was externally
 *       called to be removed due to an external issue.
 */
void lru_remove_entry(struct cache_nucleus *cn, struct entry *e) {
  struct lru_entry *le = to_lru_entry(e);

  if (!e->migrating) {
    queue_remove(&le->lru_list);
    inc_demotions(&cn->stats);
  }
  cache_nucleus_remove(cn, e);
}

/** 3.5.2 lru_evict()
 *
 * Evict the least recently used entry from LRU's queue.
 *
 * NOTE: Also fills in cache_nucleus_result information regarding what was
 *       evicted and whether it was dirty.
 */
void lru_evict(struct lru_policy *lru, struct cache_nucleus_result *result) {
  struct cache_nucleus *cn = &lru->nucleus;
  struct lru_entry *le =
      queue_pop_entry(&lru->lru_q, struct lru_entry, lru_list);
  struct entry *demoted = &le->e;

  result->old_oblock = demoted->oblock;
  result->dirty_eviction = demoted->dirty;

  lru_remove_entry(cn, demoted);
}

/** 3.5.3 lru_insert()
 *
 * "Inserts" the entry into the cache.
 *
 * NOTE: Assumes the entry will be migrating. As such, it allocates space for
 * the entry in the cache, but does *NOT* put the entry into the LRU queue.
 */
void lru_insert(struct lru_policy *lru, oblock_t oblock,
                struct cache_nucleus_result *result) {
  struct cache_nucleus *cn = &lru->nucleus;
  struct entry *e = insert_in_cache(cn, oblock, result);
  struct lru_entry *le = to_lru_entry(e);

  migrating_entry(&cn->cache_pool, e);
}

/** 3.5.4 lru_hit()
 *
 * Process a hit to the LRU; put the entry in the MRU position in LRU's queue.
 *
 * NOTE: Says where in the cache the entry is.
 * NOTE: Makes sure not to put migrating entry into LRU's queue.
 */
void lru_hit(struct lru_policy *lru, oblock_t oblock,
             struct cache_nucleus_result *result) {
  struct cache_nucleus *cn = &lru->nucleus;
  struct entry *e = hash_lookup(&cn->ht, oblock);
  struct lru_entry *le = to_lru_entry(e);

  result->cblock = infer_cblock(&cn->cache_pool, e);

  if (!e->migrating) {
    queue_remove(&le->lru_list);
    queue_push(&lru->lru_q, &le->lru_list);
  }
}

/** 3.5.5 lru_transaction_commit()
 *
 * Commit changes in lru_transaction to LRU.
 */
void lru_transaction_commit(struct lru_policy *lru, struct lru_transaction *lt,
                            struct cache_nucleus_result *result) {
  struct cache_nucleus *cn = &lru->nucleus;

  if (lt->hit) {
    result->op = CACHE_NUCLEUS_HIT;
    lru_hit(lru, lt->oblock, result);
    inc_hits(&cn->stats);
  } else {
    result->op = CACHE_NUCLEUS_NEW;
    if (lt->evict) {
      result->op = CACHE_NUCLEUS_REPLACE;
      lru_evict(lru, result);
    }
    lru_insert(lru, lt->oblock, result);
    inc_misses(&cn->stats);
  }
}

/** 4.0 Non-Default Cache Nucleus API Functions
 *
 * While Cache Nucleus has many API functions, many caching algorithms can
 * usually just use the default version of the API that only knows about the
 * Cache Nucleus.
 *
 * There are a few API functions that are essentially required to be written for
 * each caching algorithm as they require more nuance to do:
 *   remove()
 *   insert()
 *   migrated()
 *   map()
 *   destroy()
 *   next_victim()
 *
 * For more details on the API functions, check out cache_nucleus_api.h
 */

/** 4.1 lru_remove_api()
 *
 * Remove the given entry from LRU.
 *
 * NOTE: remove_to_history is required for support for certain admission
 *       policies that may want to fully remove an entry from the cache without
 *       moving it to metadata.
 *       Of course, LRU doesn't use any extra metadata space, but it's worth
 *       mentioning to understand the API.
 */
void lru_remove_api(struct cache_nucleus *cn, oblock_t oblock,
                    bool remove_to_history) {
  struct entry *e = hash_lookup(&cn->ht, oblock);
  LOG_ASSERT(e != NULL);
  lru_remove_entry(cn, e);
}

/** 4.2 lru_insert_api()
 *
 * LRU inserts the given entry into the cache at a particular spot.
 *
 * NOTE: This is useful for preloading the cache with entries, such as recovery.
 *       In this case it assumes that the entry already exists in the cache, so
 *       tracking migration here is unnecessary.
 */
void lru_insert_api(struct cache_nucleus *cn, oblock_t oblock,
                    cblock_t cblock) {
  struct lru_policy *lru = to_lru_policy(cn);
  struct entry *e = insert_in_cache_at(cn, oblock, cblock, NULL);
  struct lru_entry *le = to_lru_entry(e);

  queue_push(&lru->lru_q, &le->lru_list);
}

/** 4.3 lru_migrated_api()
 *
 * The given entry has finished migrating. Add it to LRU's queue.
 *
 *  NOTE: Promotions to cache only counted when entry is finally written to the
 *        cache after migration.
 */
void lru_migrated_api(struct cache_nucleus *cn, oblock_t oblock) {
  struct lru_policy *lru = to_lru_policy(cn);
  struct entry *e = hash_lookup(&cn->ht, oblock);
  struct lru_entry *le = to_lru_entry(e);

  migrated_entry(&cn->cache_pool, e);
  queue_push(&lru->lru_q, &le->lru_list);

  inc_promotions(&cn->stats);
}

/** 4.4 lru_map_api()
 *
 * Process request to "map" the block address to the cache.
 *
 * NOTE: We are currently doing "dirty work" at the end, marking the newly
 *       cached items as dirty so that we can note a dirty eviction happening
 *       internally to the caller.
 */
int lru_map_api(struct cache_nucleus *cn, oblock_t oblock, bool write,
                struct cache_nucleus_result *result) {
  struct entry *e = hash_lookup(&cn->ht, oblock);
  struct lru_policy *lru = to_lru_policy(cn);
  struct lru_transaction lt;

  lru_transaction_init(lru, &lt, oblock);
  lru_transaction_set(lru, &lt);
  if (lru_transaction_is_legal(lru, &lt)) {
    lru_transaction_commit(lru, &lt, result);

    inc_ios(&cn->stats);

    // dirty work
    {
      struct entry *cached_entry = hash_lookup(&cn->ht, oblock);
      if (in_cache(cn, cached_entry)) {
        cached_entry->dirty = cached_entry->dirty || write;
      }
    }
  } else {
    result->op = CACHE_NUCLEUS_FAIL;
  }

  return 0;
}

/** 4.5 lru_destroy_api()
 *
 * Destroy LRU and free all memory associated with it.
 *
 * NOTE: The entry_pool is initialized outside of the cache_nucleus,
 *       so we enforce exiting outside the cache_nucleus as well
 *       This is so we can allocate all of the lru_entry's
 *       but access all of the entry's within those lru_entry's
 *       without issue
 *
 * NOTE: You might notice the mem_free() call.
 *       We "hide" the memory allocation and freeing (being mem_alloc()
 *       and mem_free()) so that we can have a wrapper function that will
 *       internally change based on whether we are compiling for
 *       userspace or the kernel.
 */
void lru_destroy_api(struct cache_nucleus *cn) {
  struct lru_policy *lru = to_lru_policy(cn);
  epool_exit(&cn->cache_pool);
  policy_exit(cn);
  mem_free(lru);
}

/** 4.6 lru_next_victim_api()
 *
 * Gets the next entry to be evicted, should given oblock be accessed next.
 *
 * The next entry is saved to next_victim_p.
 * If LRU would not evict anything, or if it would be illegal for it to do so
 * with the algorithm's logic, NULL is saved to next_victim_p instead.
 *
 * Returns the cache operation that the request for oblock would cause.
 *
 * NOTE: Notice that we make sure not to modify LRU by using the Transaction
 *       Model to determine what changes would occur, then return what would
 *       happen should we do it without committing the changes.
 */
enum cache_nucleus_operation lru_next_victim_api(struct cache_nucleus *cn,
                                                 oblock_t oblock,
                                                 struct entry **next_victim_p) {
  struct lru_policy *lru = to_lru_policy(cn);
  struct lru_transaction lt;

  lt.oblock = oblock;
  *next_victim_p = NULL;

  lru_transaction_init(lru, &lt, oblock);
  lru_transaction_set(lru, &lt);
  if (lru_transaction_is_legal(lru, &lt)) {
    if (lt.hit) {
      return CACHE_NUCLEUS_HIT;
    }

    if (lt.evict) {
      struct lru_entry *le =
          queue_peek_entry(&lru->lru_q, struct lru_entry, lru_list);
      *next_victim_p = &le->e;
      return CACHE_NUCLEUS_REPLACE;
    }
    return CACHE_NUCLEUS_NEW;
  }
  return CACHE_NUCLEUS_FAIL;
}

/** 5.0 LRU Creation Functions
 *
 * These functions allocate and initialize LRU and it's associated structures.
 */

/** 5.1 lru_init()
 *
 * A helper function that initializes the lru_policy after it
 * is created!
 *
 * cache_nucleus_init_api():
 * We use a function for initializing the cache_nucleus's API
 * This helps create errors when we change it, add functions,
 * or remove functions, instead of silent bugs occurring from
 * functions in the API going without being set.
 * For simplistic functions (can almost be called optional)
 * we provide the default functions for these.
 * The reason we provide these at all is that some external
 * programs may not be accessing the policy directly, and so
 * we provide the API to help for those occassions where we're
 * accessing the policy indirectly (such as mstar)
 *
 * epool_create():
 * This is actually a macro that creates cache_size lru_entry's,
 * but sets pointers for external programs to access the entry
 * part of every lru_entry in the entry_pool.
 * This couldn't be done with a function, so we have it as a macro,
 * which means that the entry_pools must be initialized (created)
 * within the policy itself rather than the cache_nucleus!
 * You may notice that we don't create an entry_pool for meta_pool,
 * even if it would be unused or 0.
 * Unfortunately, 0 is not accepted and causes issues when working
 * with allocating using the kernel.
 */
int lru_init(struct lru_policy *lru, cblock_t cache_size) {
  struct cache_nucleus *cn = &lru->nucleus;

  cache_nucleus_init_api(
      cn, lru_map_api, lru_remove_api, lru_insert_api, lru_migrated_api,
      default_cache_lookup, default_meta_lookup, default_cblock_lookup,
      default_remap, lru_destroy_api, default_get_stats, default_set_time,
      default_residency, default_cache_is_full, default_infer_cblock,
      lru_next_victim_api);

  queue_init(&lru->lru_q);

  epool_create(&cn->cache_pool, cache_size, struct lru_entry, e);
  if (!cn->cache_pool.entries_begin || !cn->cache_pool.entries) {
    LOG_DEBUG("epool_create for cache_pool failed");
    goto cache_epool_create_fail;
  }

  if (policy_init(cn, cache_size, 0, CACHE_NUCLEUS_SINGLE_THREAD)) {
    LOG_DEBUG("policy_init failed");
    goto policy_init_fail;
  }

  return 0;

policy_init_fail:
  epool_exit(&cn->cache_pool);
cache_epool_create_fail:
  return -ENOSPC;
}

/** 5.2 lru_create()
 *
 * Create an lru_policy, and return the internal cache_nucleus for the
 * external program to interact with.
 *
 * NOTE: This represents the "C-Style Polymorphism" design of Cache Nucleus,
 *       where we're providing an interface (cache_nucleus) that is guaranteed
 *       and has defined functions (the API) that we know we should be able to
 *       interact with the algorithm (like LRU) without the caller knowing
 *       anything about the actual algorithm.
 */
struct cache_nucleus *lru_create(cblock_t cache_size, cblock_t meta_size) {
  int r;
  struct cache_nucleus *cn;
  struct lru_policy *lru = mem_alloc(sizeof(*lru));
  if (!lru) {
    LOG_DEBUG("mem_alloc failed. insufficient memory");
    goto lru_create_fail;
  }

  cn = &lru->nucleus;

  r = lru_init(lru, cache_size);
  if (r) {
    LOG_DEBUG("lru_init failed. error %d", r);
    goto lru_init_fail;
  }

  return cn;

lru_init_fail:
  lru_destroy_api(cn);
lru_create_fail:
  return NULL;
}
