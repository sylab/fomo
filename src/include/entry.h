#ifndef INCLUDE_BASE_ENTRY_H
#define INCLUDE_BASE_ENTRY_H

#include "common.h"
#include "kernel/hash.h"
#include "kernel/list.h"
#include "tools/queue.h"

/** A cache entry.
 *
 * A general implementation of a cache entry struct that is meant to be visible
 * from the base_policy, entry_pool, and other algorithm-agnostic tools. It is
 * intended to be included within an algorithm-specific struct to provide
 * "C-style polymorphism".
 *
 * ht_list - A hlist_node for hashtable use
 * wb_list - A list_head (essentially) for writeback_tracker queue use
 *           (used externally to base_policy)
 * pool_list - A list_head for entry_pool use
 * oblock - Origin device block address
 * size - Size of entry, measured in units
 * time - Track the last time accessed (used externally to base_policy)
 * dirty - Track whether the entry is dirty
 * allocated - Track whether the entry has been allocated or not
 * migrating - States whether the entry is in the process of migrating
 */
struct entry {
  struct hlist_node ht_list;
  struct queue_head wb_list;
  struct list_head pool_list;
  oblock_t oblock;

  unsigned size;
  unsigned time;

  bool dirty : 1;
  bool allocated : 1;
  bool migrating : 1;
};

#endif /* INCLUDE_BASE_ENTRY_H */
