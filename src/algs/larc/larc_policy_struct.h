#ifndef ALGS_LARC_LARC_POLICY_STRUCT_H
#define ALGS_LARC_LARC_POLICY_STRUCT_H

#include "cache_nucleus_struct.h"
#include "common.h"
#include "entry.h"
#include "tools/queue.h"

/** larc_policy:
 * larc_q - LARC's LRU queue
 * larc_g - LARC's LRU "ghost" queue
 * g_size - size of LARC's "ghost" queue
 */
struct larc_policy {
  struct cache_nucleus nucleus;
  struct queue larc_q;
  struct queue larc_g;
  cblock_t g_size;
};

struct larc_entry {
  struct entry e;
  struct queue_head larc_list;
};

static struct larc_policy *to_larc_policy(struct cache_nucleus *cn) {
  return container_of(cn, struct larc_policy, nucleus);
}

static struct larc_entry *to_larc_entry(struct entry *e) {
  return container_of(e, struct larc_entry, e);
}

#endif /* ALGS_LARC_LARC_POLICY_STRUCT_H */
