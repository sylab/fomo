#ifndef INCLUDE_POLICY_STATS_H
#define INCLUDE_POLICY_STATS_H

/** policy_stats:
 * Statistics for the policy to keep a record of
 *
 * hits - How often an entry is in the cache when accessed
 * misses - How often an entry is *not* in the cache when accessed
 * filters - How often an access is missed, but not cached
 * promotions - How often an entry is moved from origin device to cache device
 * demotions - How often an entry is moved from cache device to origin device
 */
struct policy_stats {
  unsigned ios;
  unsigned hits;
  unsigned misses;
  unsigned filters;
  unsigned promotions;
  unsigned demotions;
  unsigned private_stat;
};

static void stats_init(struct policy_stats *stats) {
  stats->ios = 0;
  stats->hits = 0;
  stats->misses = 0;
  stats->filters = 0;
  stats->promotions = 0;
  stats->demotions = 0;
  stats->private_stat = 0;
}

static void inc_ios(struct policy_stats *stats) { ++stats->ios; }
static void inc_hits(struct policy_stats *stats) { ++stats->hits; }
static void inc_misses(struct policy_stats *stats) { ++stats->misses; }
static void inc_filters(struct policy_stats *stats) { ++stats->filters; }
static void inc_promotions(struct policy_stats *stats) { ++stats->promotions; }
static void inc_demotions(struct policy_stats *stats) { ++stats->demotions; }
static void inc_private_stat(struct policy_stats *stats) {
  ++stats->private_stat;
}

#endif /* INCLUDE_POLICY_STATS_H */
