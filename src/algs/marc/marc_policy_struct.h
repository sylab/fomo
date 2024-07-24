#ifndef ALGS_MARC_MARC_POLICY_STRUCT_H
#define ALGS_MARC_MARC_POLICY_STRUCT_H

#include "cache_nucleus_struct.h"
#include "tools/queue.h"
#include "tools/window_stats.h"

enum marc_state { UNSTABLE, STABLE, UNIQUE };

struct marc_policy {
  struct cache_nucleus nucleus;

  struct cache_nucleus *internal;

  uint64_t TABU;
  uint64_t tabu_counter;

  enum marc_state state;

  struct queue ghost_q;
  uint64_t ghost_size;
  uint64_t ghost_hits;
  uint64_t ghost_hits_old;

  struct window_stats sample;
  struct window_stats sample_avg;

  uint64_t phase_hits;
  uint64_t phase_ios;
};

struct marc_entry {
  struct entry e;
  struct queue_head marc_list;
};

static struct marc_policy *to_marc_policy(struct cache_nucleus *cn) {
  return container_of(cn, struct marc_policy, nucleus);
}

static struct marc_entry *to_marc_entry(struct entry *e) {
  return container_of(e, struct marc_entry, e);
}

#endif /* ALGS_MARC_MARC_POLICY_STRUCT_H */
