#ifndef INCLUDE_TOOL_HASHTABLE_H
#define INCLUDE_TOOL_HASHTABLE_H

#include "common.h"
#include "entry.h"
#include "kernel/hash.h"
#include "kernel/list.h"
#ifndef __KERNEL__
#include <errno.h>
#include <strings.h> // ffs()

static unsigned next_power(unsigned n, unsigned min) {
  unsigned x = max(n, min);
  unsigned pow = 0;
  while (x > 1 << pow)
    ++pow;
  return 1 << pow;
}
#else
static unsigned next_power(unsigned n, unsigned min) {
  return roundup_pow_of_two(max(n, min));
}
#endif

struct entry;

/** A hashtable struct that is general but used for entries
 *
 * This hashtable implementation only requires that the struct stored as the
 * value contain a hlist_node member and that the key is located within the
 * struct for comparisons during a search of the hashtable.
 *
 * \note This implementation is a chaining hashtable, so conflicts simply get
 *       chained together within the same hashtable "bucket".
 *
 * \warning While this is a generalized hashtable, the functions accompanying
 *          it are written with entries in mind.
 *
 * nr_buckets - Size of the hashtable, or how many "buckets" it has
 * hash_bits - A bitmask for the bits to guarantee that it has a place in the
 *             hashtable
 * table - Array of lists (a.k.a an array of "buckets")
 */
struct hashtable {
  unsigned nr_buckets;
  block_t hash_bits;
  struct hlist_head *table;
};

/** Initalize the hashtable
 *
 * \note In order to have a bitmask for hashing for placing the entries in the
 *       proper bucket, we must have 2^n buckets.
 * \note With this kind of requirement, we set nr_buckets to be a power of 2,
 *       and utilize this size to create the bitmask by simply subtracting 1.
 *
 * \return 0 if no errors occur, or -ENOSPC if unable to allocate memory
 */
static int ht_init(struct hashtable *ht, block_t cache_size) {
  ht->nr_buckets = next_power(from_cblock(cache_size) / 2, 16);
  ht->hash_bits = ffs(ht->nr_buckets) - 1;
  ht->table = mem_alloc(sizeof(*ht->table) * ht->nr_buckets);
  if (ht->table == NULL)
    return -ENOSPC;

  return 0;
}

/** Insert an entry into the hashtable
 */
static void hash_insert(struct hashtable *ht, struct entry *e) {
  unsigned h = hash_64(from_oblock(e->oblock), ht->hash_bits);
  hlist_add_head(&e->ht_list, ht->table + h);
}

/** Lookup an entry from the hashtable
 *
 * \note As seen from the implementation, when found, the entry is put first
 *       in the bucket to make the next lookup for it faster.
 *
 * \return Pointer to the entry we're looking for if found, NULL if not.
 */
static struct entry *hash_lookup(struct hashtable *ht, oblock_t oblock) {
  unsigned h = hash_64(from_oblock(oblock), ht->hash_bits);
  struct hlist_head *bucket = ht->table + h;
  struct entry *e;

  hlist_for_each_entry(e, bucket, ht_list) {
    if (e->oblock == oblock) {
      hlist_del(&e->ht_list);
      hlist_add_head(&e->ht_list, bucket);
      return e;
    }
  }

  return NULL;
}

/** Remove an entry from the hashtable
 *
 * \note Since the hlist_node in the entry is essentially a doubly-linked list
 *       node, we simply follow the same procedure as that kind of removal and
 *       \a voila: the entry is safely removed from the hashtable it was in
 *       without making any other entries unreachable.
 */
static void hash_remove(struct entry *e) { hlist_del(&e->ht_list); }

/** Exit (or free) the hashtable
 *
 * \note This does not reset all of the entries to not point back to the
 *       hashtable's buckets.
 *       However, this should be done only when a policy and therefore all
 *       associated entries are being freed as well.
 */
static void hash_exit(struct hashtable *ht) { mem_free(ht->table); }

#endif /* INCLUDE_TOOL_HASHTABLE_H */
