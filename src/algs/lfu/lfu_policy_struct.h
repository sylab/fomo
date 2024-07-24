#ifndef ALGS_LFU_LFU_POLICY_STRUCT_H
#define ALGS_LFU_LFU_POLICY_STRUCT_H

#include "cache_nucleus_struct.h"
#include "common.h"
#include "entry.h"
#include "tools/heap.h"

struct lfu_policy {
  struct cache_nucleus nucleus;
  struct heap heap;
};

struct lfu_entry {
  struct entry e;
  struct heap_head lfu_list;
};

static struct lfu_policy *to_lfu_policy(struct cache_nucleus *cn) {
  return container_of(cn, struct lfu_policy, nucleus);
}

static struct lfu_entry *to_lfu_entry(struct entry *e) {
  return container_of(e, struct lfu_entry, e);
}

#endif /* ALGS_LFU_LFU_POLICY_STRUCT_H */
