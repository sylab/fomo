#ifndef ALGS_ARC_ARC_POLICY_STRUCT_H
#define ALGS_ARC_ARC_POLICY_STRUCT_H

#include "cache_nucleus_struct.h"
#include "common.h"
#include "entry.h"
#include "tools/queue.h"

#define NUM_ARC_LISTS 4
enum arc_list { ARC_T1, ARC_T2, ARC_B1, ARC_B2, ARC_NULL };

struct arc_policy {
  struct cache_nucleus nucleus;
  struct queue arc_q[NUM_ARC_LISTS];
  cblock_t p;
  cblock_t length[NUM_ARC_LISTS];
};

struct arc_entry {
  struct entry e;
  struct queue_head arc_list;
  enum arc_list loc;
};

static struct arc_policy *to_arc_policy(struct cache_nucleus *cn) {
  return container_of(cn, struct arc_policy, nucleus);
}

static struct arc_entry *to_arc_entry(struct entry *e) {
  return container_of(e, struct arc_entry, e);
}

#endif /* ALGS_ARC_ARC_POLICY_STRUCT_H */
