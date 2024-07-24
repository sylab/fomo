#ifndef MSTAR_MSTAR_POLICY_STRUCT_H
#define MSTAR_MSTAR_POLICY_STRUCT_H

#include "mstar_logic.h"

/** m* caching policy
 *
 *
 */
struct mstar_policy {
  struct cache_nucleus nucleus;
  struct queue mstar_g;
  cblock_t ghost_size;
  struct cache_nucleus *internal_policy;
  struct mstar_logic logic;
};

/** m* entry
 *
 */
struct mstar_entry {
  struct entry e;
  struct queue_head mstar_list;
};

static struct mstar_policy *to_mstar_policy(struct cache_nucleus *cn) {
  return container_of(cn, struct mstar_policy, nucleus);
}

static struct mstar_entry *to_mstar_entry(struct entry *e) {
  return container_of(e, struct mstar_entry, e);
}

#endif /* MSTAR_MSTAR_POLICY_STRUCT_H */
