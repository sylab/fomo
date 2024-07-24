#ifndef ALGS_LRU_LRU_POLICY_STRUCT_H
#define ALGS_LRU_LRU_POLICY_STRUCT_H

#include "cache_nucleus_struct.h"
#include "common.h"
#include "entry.h"
#include "tools/queue.h"

/** lru_policy:
 * Every policy should include cache_nucleus as a member to have a
 * common collection of structures and API functions for external
 * programs to interact with LRU without knowing *anything* about
 * LRU!
 * If you want to know more about cache_nucleus and what it covers,
 * check out the source code in src/include/cache_nucleus.h!
 *
 * Other than that, we include anything LRU specific in the struct,
 * which is just the LRU queue for tracking recency in this case!
 * If you want to know more about the queue implementation,
 * check out the source code in src/include/tools/queue.h!
 */
struct lru_policy {
  struct cache_nucleus nucleus;
  struct queue lru_q;
};

/** lru_entry:
 * Every entry should include entry as a member to have a common
 * collection of structures and members for external programs to
 * interact with entries without knowing anything about LRU!
 *
 * Other than that, we include anything LRU specific in the struct,
 * which is just a queue_head, which is how lru_entry's keep
 * track of where they are in the queue.
 */
struct lru_entry {
  struct entry e;
  struct queue_head lru_list;
};

/** to_lru_policy:
 * A helpful function that converts a cache_nucleus pointer to
 * a pointer to the containing lru_policy
 */
static struct lru_policy *to_lru_policy(struct cache_nucleus *cn) {
  return container_of(cn, struct lru_policy, nucleus);
}

/** to_lru_entry:
 * A helpful function that converts an entry pointer to
 * a pointer to the containing lru_entry
 */
static struct lru_entry *to_lru_entry(struct entry *e) {
  return container_of(e, struct lru_entry, e);
}

#endif /* ALGS_LRU_LRU_POLICY_STRUCT_H */
