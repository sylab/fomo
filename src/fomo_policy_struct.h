#ifndef FOMO_FOMO_POLICY_STRUCT_H
#define FOMO_FOMO_POLICY_STRUCT_H

#include "tools/queue.h"

enum fomo_state { FOMO_FILTER, FOMO_INSERT };

struct fomo_period {
  struct queue_head list;
  unsigned c_hits;
  unsigned mh_hits;
};

struct fomo_policy {
  struct cache_nucleus nucleus;
  struct queue fomo_mh;
  cblock_t ghost_size;
  struct cache_nucleus *internal_policy;
  enum fomo_state state;

  // TODO sliding window
  //      Not updated on illegal?
  cblock_t counter;

  cblock_t period_size;
  uint64_t c_hits;
  uint64_t mh_hits;

  // for wrapper
  unsigned mh_hits_total;
};

struct fomo_entry {
  struct entry e;
  struct queue_head fomo_list;
};

static struct fomo_policy *to_fomo_policy(struct cache_nucleus *cn) {
  return container_of(cn, struct fomo_policy, nucleus);
}

static struct fomo_entry *to_fomo_entry(struct entry *e) {
  return container_of(e, struct fomo_entry, e);
}

static struct cache_nucleus *get_internal_policy(struct cache_nucleus *cn) {
  struct fomo_policy *fomo = to_fomo_policy(cn);
  return fomo->internal_policy;
}

#endif /* FOMO_FOMO_POLICY_STRUCT_H */
