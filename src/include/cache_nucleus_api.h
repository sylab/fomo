/* policy API:
 * External facing info/functions for cache_nucleus for API purposes mostly
 */

#ifndef POLICY_API_H
#define POLICY_API_H

#include "cache_nucleus_struct.h"
#include "entry.h"
#include "policy_stats.h"
#include "tools/epool.h"
#include "tools/hashtable.h"

// API Functions

/** policy_remove:
 * Completely removes the cached -OR- meta entry with the given oblock from the
 * algorithm (Typically as part of error correction).
 * Should the removed entry be that of a cached entry, remove_to_history says
 * whether the entry is to be moved to the history of the policy or not.
 */
static void policy_remove(struct cache_nucleus *cn, oblock_t oblock,
                          bool remove_to_history) {
  cn->remove(cn, oblock, remove_to_history);
}

/** policy_insert:
 * Insert the entry with the given oblock into the cache using a default method
 * of insertion.
 */
static void policy_insert(struct cache_nucleus *cn, oblock_t oblock,
                          cblock_t cblock) {
  cn->insert(cn, oblock, cblock);
}

/** policy_migrated:
 * Mark the given oblock as done migrating.
 * The algorithm should now treat the entry as valid and in the cache.
 */
static void policy_migrated(struct cache_nucleus *cn, oblock_t oblock) {
  cn->migrated(cn, oblock);
}

/** policy_cache_lookup:
 * Lookup cached entry with given oblock.
 */
static struct entry *policy_cache_lookup(struct cache_nucleus *cn,
                                         oblock_t oblock) {
  return cn->cache_lookup(cn, oblock);
}

/** policy_meta_lookup:
 * Lookup meta entry with given oblock.
 */
static struct entry *policy_meta_lookup(struct cache_nucleus *cn,
                                        oblock_t oblock) {
  return cn->meta_lookup(cn, oblock);
}

/** policy_cblock_lookup:
 * Lookup entry (allocated or not) with given cblock.
 */
static struct entry *policy_cblock_lookup(struct cache_nucleus *cn,
                                          cblock_t cblock) {
  return cn->cblock_lookup(cn, cblock);
}

/** policy_map:
 * The algorithm decides what to do when asked to acess the given oblock.
 * The algorithm puts this information in result and passed back to the caller.
 * Returns 0 if no problems occur, otherwise an error occurred.
 */
static int policy_map(struct cache_nucleus *cn, oblock_t oblock, bool write,
                      struct cache_nucleus_result *result) {
  return cn->map(cn, oblock, write, result);
}

/** policy_remap:
 * The entry identified with current_oblock will now be identified with
 * new_oblock.
 * NOTE: This allows for multi-layered policies to have meta entries also
 *       properly remap
 */
static void policy_remap(struct cache_nucleus *cn, oblock_t current_oblock,
                         oblock_t new_oblock) {
  cn->remap(cn, current_oblock, new_oblock);
}

/** policy_destroy:
 * Free up memory space taken up by the policy.
 */
static void policy_destroy(struct cache_nucleus *cn) { cn->destroy(cn); }

/** policy_get_stats:
 * Get the stats for the policy.
 * NOTE: A policy_stats struct is passed in so that multi-layered policies can
 *       aggregate stats to provide an accurate picture of the current stats of
 *       the policy.
 */
static void policy_get_stats(struct cache_nucleus *cn,
                             struct policy_stats *stats) {
  return cn->get_stats(cn, stats);
}

/** policy_set_time:
 * Set the time for the policy.
 * While IO count is usually used internally for policies, some may prefer to
 * use whatever timing mechanism the external running application may be using
 * (such as dm-cache's ticks)
 */
static void policy_set_time(struct cache_nucleus *cn, unsigned time) {
  cn->set_time(cn, time);
}

/** policy_residency:
 * How much of the cache is allocated/used?
 */
static cblock_t policy_residency(struct cache_nucleus *cn) {
  return cn->residency(cn);
}

/** policy_cache_is_full:
 * Is the cache full?
 */
static bool policy_cache_is_full(struct cache_nucleus *cn) {
  return cn->cache_is_full(cn);
}

/** policy_infer_cblock:
 * Get cblock of given entry.
 */
static cblock_t policy_infer_cblock(struct cache_nucleus *cn, struct entry *e) {
  return cn->infer_cblock(cn, e);
}

/** policy_next_victim:
 * Get entry to be evicted next.
 */
static enum cache_nucleus_operation
policy_next_victim(struct cache_nucleus *cn, oblock_t oblock,
                   struct entry **next_victim_p) {
  return cn->next_victim(cn, oblock, next_victim_p);
}

#endif /* POLICY_API_H */
