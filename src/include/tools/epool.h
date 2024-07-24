#ifndef INCLUDE_TOOL_EPOOL_H
#define INCLUDE_TOOL_EPOOL_H

#include "common.h"
#include "entry.h"
#include "kernel/hash.h"
#include "kernel/list.h"
#include "tools/queue.h"

static struct list_head *list_pop(struct list_head *lh) {
  struct list_head *r = lh->next;

  LOG_ASSERT(r);
  list_del_init(r);

  return r;
}

struct entry;

/** Pool of memory to contain entries for policies
 *
 * While we could implement our policies so that we simply free and allocate
 * entries as they leave and enter caches, this can be rather slow and we don't
 * have a guarantee during runtime that the memory would be available for us.
 * Creating a pool of memory during creation sidesteps this problem.
 * During creation, we need policy-specific entry information to allocate the
 * right amount of memory, as well as where the "base" (or "general") entry
 * structure is within the policy-specific entry for allowing public access
 * to the "general" entry information while hiding the policy-specific bits.
 *
 * entries_begin - Pointer to the first "general" entry
 * entries_end - Pointer to the final "general" entry
 * free - List of free, "unallocated" entries
 * migrating - List of "migrating" entries
 * nr_allocated - Number of "allocated" entries
 * nr_migrating - Number of "migrating" entries
 * nr_entries - Number of total entries in the pool
 * entries - Array of pointers to "general" entries
 */
struct entry_pool {
  void *entries_begin;
  void *entries_end;
  struct list_head free;
  struct list_head migrating;
  unsigned nr_allocated;
  unsigned nr_migrating;
  unsigned nr_entries;
  unsigned starting_index;
  struct entry **entries;
};

/** Allocate memory and create the pool for entries
 *
 * Uses information known only to the policy to create an entry pool.
 *
 * This information includes:
 * - policy-specific entry struct
 * - identifier for "general" entry in policy-specific entry struct
 *
 * \note Due to this requirement for specialized information, the function
 *         could only be written as a macro.
 *
 * After the call is done, the entry_pool (_epool) should have:
 * - entries_begin point to the first policy-specific entry
 * - entries_end point to the last policy-specific entry
 * - nr_entries be set to _nr_entries
 * - entries pointing to the "general" entry part of all policy-specific entries
 *
 * \warning Caller \a NEEDS to check after for null entries_begin and/or
 *          ep->entries
 *
 * All that's left is to call epool_init() to put all the entries in the free
 * list to be grabbed on demand.
 * The reason it's not done here?
 * We could, but if we're trying to be honest, the separation of the creation
 * with the initialization allows for the possibility to reset the entry_pool
 * without needing to destroy then recreate the entry_pool to do so.
 *
 * \note We could also say that epool_create() only works with what it
 *       actually initializes (with nr_entries being the exception).
 *
 * _epool - Pointer to an entry_pool
 * _nr_entries - Number of entries the entry_pool should have
 * _type - The type of the policy-specific entry
 * _member - The "general" entry member of the policy-specific entry
 */
#define epool_create(_epool, _nr_entries, _type, _member)                      \
  {                                                                            \
    struct entry_pool *ep_ = (_epool);                                         \
    unsigned nr_ = (_nr_entries);                                              \
    unsigned i_;                                                               \
    ep_->entries_begin = mem_alloc(sizeof(_type) * nr_);                       \
    ep_->entries_end = ((_type *)ep_->entries_begin) + nr_;                    \
    ep_->nr_entries = nr_;                                                     \
    ep_->starting_index = 0;                                                   \
    ep_->entries = mem_alloc(sizeof(*ep_->entries) * nr_);                     \
    if (ep_->entries_begin && ep_->entries) {                                  \
      for (i_ = 0; i_ < nr_; i_++) {                                           \
        ep_->entries[i_] = &((_type *)ep_->entries_begin)[i_]._member;         \
      }                                                                        \
    }                                                                          \
  }

/** Get the entry at a particular index
 */
static struct entry *epool_at(struct entry_pool *ep, cblock_t index) {
  cblock_t converted_index = index - ep->starting_index;
  if (converted_index >= ep->nr_entries)
    return NULL;

  return ep->entries[converted_index];
}

/** Initializes an entry_pool
 *
 * \note Called after an #epool_create()
 *
 * Initalizes the entry_pool by taking the "general" entries pointed at by the
 * entries array and adding them all to a re-initialized free list.
 *
 * TODO talk about starting_index
 *
 * Also resets the nr_allocated member.
 */
static void epool_init(struct entry_pool *ep, unsigned starting_index) {
  unsigned i;

  INIT_LIST_HEAD(&ep->free);
  INIT_LIST_HEAD(&ep->migrating);
  for (i = 0; i < ep->nr_entries; i++) {
    struct entry *e = epool_at(ep, i);

    INIT_HLIST_NODE(&e->ht_list);
    INIT_LIST_HEAD(&e->wb_list.list);
    e->wb_list.parent_queue = NULL;
    list_add(&e->pool_list, &ep->free);

    e->dirty = false;
    e->allocated = false;
    e->migrating = false;
  }
  ep->nr_allocated = 0;
  ep->nr_migrating = 0;
  ep->starting_index = starting_index;
}

/** Exits (frees) the memory allocated for the entry_pool
 */
static void epool_exit(struct entry_pool *ep) {
  if (ep->entries_begin != NULL) {
    mem_free(ep->entries_begin);
  }
  if (ep->entries != NULL) {
    mem_free(ep->entries);
  }
}

/** "Allocates" the given entry from the entry_pool
 */
static void alloc_this_entry(struct entry_pool *ep, struct entry *e) {
  list_del_init(&e->pool_list);
  INIT_HLIST_NODE(&e->ht_list);
  queue_head_init(&e->wb_list);
  e->allocated = true;
  ++ep->nr_allocated;
}

/** "Allocates" an entry from the entry_pool
 *
 * Gets a free entry from the entry_pool's free entry list.
 * Should the free entry list be empty, a NULL pointer is returned.
 */
static struct entry *alloc_entry(struct entry_pool *ep) {
  struct entry *e;

  if (list_empty(&ep->free))
    return NULL;

  e = list_entry(list_pop(&ep->free), struct entry, pool_list);
  alloc_this_entry(ep, e);
  return e;
}

/** "Allocates" a particular entry from the entry_pool
 *
 * Gets the entry from the entry_pool should the entry be free.
 *
 * \warning It is assumed that the caller knows what they're doing and should
 *          the entry already be allocated, the program fails
 */
static struct entry *alloc_particular_entry(struct entry_pool *ep,
                                            cblock_t cblock) {
  struct entry *e = epool_at(ep, from_cblock(cblock));
  LOG_ASSERT(!e->allocated);

  alloc_this_entry(ep, e);
  return e;
}

/** Find an "allocated" entry
 */
static struct entry *epool_find(struct entry_pool *ep, cblock_t cblock) {
  struct entry *e = epool_at(ep, from_cblock(cblock));
  return e = e->allocated ? e : NULL;
}

/** Is the entry_pool free list empty?
 */
static bool epool_empty(struct entry_pool *ep) { return list_empty(&ep->free); }

/** Is the entry from this entry_pool?
 *
 * \note We're using (void *) to use policy-specific entry \a or "general" entry
 * for in_pool() check
 */
static bool in_pool(struct entry_pool *ep, void *e) {
  return e >= ep->entries_begin && e < ep->entries_end;
}

/** Infer the cache block address of the entry
 *
 * Using the memory address of the entry, infer the cache block address of the
 * entry.
 *
 * \note We have to cast to uintptr_t in order to do arithmetic.
 *       We're not allowed to do that with (void *) and division is not allowed
 *       with other pointers.
 *       This allows a clean, understandable cast that is an
 *       unsigned integer usable as a cache block address at the end.
 *
 * \warning If entry is \a not from the entry_pool the program fails.
 */
static cblock_t infer_cblock(struct entry_pool *ep, struct entry *e) {
  uintptr_t entries_begin;
  uintptr_t space_between;
  LOG_ASSERT(ep != NULL);
  LOG_ASSERT(in_pool(ep, e));
  entries_begin = (uintptr_t)ep->entries[0];
  space_between = ((uintptr_t)ep->entries[1]) - entries_begin;
  return ((((uintptr_t)e) - entries_begin) / space_between) +
         ep->starting_index;
}

/** "Reserve" the entry from the entry_pool
 *
 * Moves the entry to the entry_pool's migrating list
 *
 * \warning Make sure that necessary algorithm information is still
 *          in the entry for when it is removed from the migrating list
 */
static void migrating_entry(struct entry_pool *ep, struct entry *e) {
  LOG_ASSERT(ep != NULL);
  LOG_ASSERT(in_pool(ep, e));
  LOG_ASSERT(ep->nr_allocated > 0);
  LOG_ASSERT(e->allocated);
  e->migrating = true;
  ++ep->nr_migrating;
  list_add(&e->pool_list, &ep->migrating);
}

/** Entry has "migrated", or finished "migrating"
 *
 * Removes the entry from the entry_pool's migrating list
 */
static void migrated_entry(struct entry_pool *ep, struct entry *e) {
  LOG_ASSERT(ep != NULL);
  LOG_ASSERT(in_pool(ep, e));
  LOG_ASSERT(ep->nr_allocated > 0);
  LOG_ASSERT(e->allocated);
  LOG_ASSERT(e->migrating);
  e->migrating = false;
  --ep->nr_migrating;
  list_del_init(&e->pool_list);
}

/** "Free" the entry from the entry_pool
 *
 * De-allocates the entry and adds it to the entry_pool's free list
 *
 * \warning If entry is \a not from the entry_pool \b or if the entry_pool
 *          doesn't have any entries allocated the program fails.
 */
static void free_entry(struct entry_pool *ep, struct entry *e) {
  LOG_ASSERT(ep != NULL);
  LOG_ASSERT(in_pool(ep, e));
  LOG_ASSERT(ep->nr_allocated > 0);
  ep->nr_allocated--;
  e->allocated = false;
  if (e->migrating) {
    ep->nr_migrating--;
    e->migrating = false;
    list_del_init(&e->pool_list);
  }
  INIT_HLIST_NODE(&e->ht_list);
  list_add(&e->pool_list, &ep->free);
}

#endif /* INCLUDE_TOOL_EPOOL_H */
