#ifndef SIM_POLICY_CACHE_NUCLEUS_H
#define SIM_POLICY_CACHE_NUCLEUS_H

#include "cache_nucleus.h"

static int sim_policy_map(struct cache_nucleus *bp, oblock_t oblock, bool write,
                          struct cache_nucleus_result *result) {
  return bp->map(bp, oblock, write, result);
}

static void sim_policy_migrated(struct cache_nucleus *bp, oblock_t oblock) {
  bp->migrated(bp, oblock);
}

static void sim_policy_get_stats(struct cache_nucleus *bp,
                                 struct policy_stats *stats) {
  bp->get_stats(bp, stats);
}

static void sim_policy_set_time(struct cache_nucleus *bp, unsigned time) {
  bp->set_time(bp, time);
}

static void sim_policy_destroy(struct cache_nucleus *bp) { bp->destroy(bp); }

#endif /* SIM_POLICY_CACHE_NUCLEUS_H */
