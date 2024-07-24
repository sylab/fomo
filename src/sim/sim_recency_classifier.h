#ifndef SIM_SIM_RECENCY_CLASSIFIER_H
#define SIM_SIM_RECENCY_CLASSIFIER_H

#include "cache_nucleus.h"
#include "common.h"
#include "entry.h"
#include "tools/hashtable.h"
#include "tools/queue.h"

/** TODO description
 * should start with ENTRY_ACCESSED = 0
 */
#define NUM_RECENCY_TYPES 4
enum recency_type {
  ENTRY_ACCESSED,
  ENTRY_INSERTED,
  ENTRY_EVICTED,
  ENTRY_FILTERED,
  ENTRY_FAIL,
};

struct recency_classifier_entry {
  struct entry e;
  struct queue_head list[NUM_RECENCY_TYPES];
  unsigned recency_flags;
};

struct recency_classifier {
  struct hashtable ht;
  /* size == NUM_RECENCY_TYPES * cache_size + 2
   * reserve space for a unique entry in each queue, with each queue of size N
   * (cache_size), along with the current access and possible evicted entry
   * before evictions that may introduce removal of orphan entry/entries
   */
  struct entry_pool entry_pool;
  struct queue q[NUM_RECENCY_TYPES];

  /* Include hit information as a bit on access
   * Ordered as such for future compatibility
   *  hit bit                 00001
   *  accessed recently bit   00010  (2 << ENTRY_ACCESSED)
   *  inserted recently bit   00100  (2 << ENTRY_INSERTED)
   *  evicted recently bit    01000  (2 << ENTRY_EVICTED)
   *  filtered recently bit   10000  (2 << ENTRY_FILTERED)
   */
  unsigned count[1 << (NUM_RECENCY_TYPES + 1)];

  cblock_t cache_size;
};

static unsigned recency_type_flag(enum recency_type r_type) {
  return 2 << r_type;
}

static struct recency_classifier_entry *
to_recency_classifier_entry(struct entry *e) {
  return container_of(e, struct recency_classifier_entry, e);
}

static void recency_classifier_print_stats(struct recency_classifier *r_class,
                                           unsigned sampling_rate) {
  int i;
  LOG_PRINT_F(stdout, "%u", r_class->count[0] * sampling_rate);
  for (i = 1; i < (1 << (NUM_RECENCY_TYPES + 1)); ++i) {
    LOG_PRINT_F(stdout, " %u", r_class->count[i] * sampling_rate);
  }
  LOG_PRINT_F(stdout, "\n");
}

static void
recency_classifier_push_to_queue(struct recency_classifier *r_class,
                                 enum recency_type r_type,
                                 struct recency_classifier_entry *re) {
  struct queue *q = &r_class->q[r_type];
  struct queue_head *qh = &re->list[r_type];

  if (in_queue(q, qh)) {
    queue_remove(qh);
  }
  queue_push(q, qh);
  re->recency_flags |= recency_type_flag(r_type);

  if (queue_length(q) > r_class->cache_size) {
    struct queue_head *qh = queue_pop(q);
    // check if entry is orphan now
    struct recency_classifier_entry *evicted_re =
        container_of(qh, struct recency_classifier_entry, list[r_type]);

    // ex: cancelling flag 00010
    //  01010 = 01010 & ~(00010)
    //        = 01010 & 11101
    //        = 01000
    evicted_re->recency_flags &= ~recency_type_flag(r_type);

    if (evicted_re->recency_flags == 0) {
      struct entry *evicted_e = &evicted_re->e;
      hash_remove(evicted_e);
      free_entry(&r_class->entry_pool, evicted_e);
    }
  }
}

static void recency_classifier_update(struct recency_classifier *r_class,
                                      oblock_t oblock,
                                      struct cache_nucleus_result *result) {
  struct recency_classifier_entry *re;
  struct recency_classifier_entry *evicted_re;
  struct entry *e;
  unsigned classification;
  int i;

  if (result->op == CACHE_NUCLEUS_FAIL) {
    return;
  }

  e = hash_lookup(&r_class->ht, oblock);
  if (e == NULL) {
    e = alloc_entry(&r_class->entry_pool);
    LOG_ASSERT(e != NULL);
    e->oblock = oblock;
    hash_insert(&r_class->ht, e);
  }
  re = to_recency_classifier_entry(e);
  classification = re->recency_flags;

  recency_classifier_push_to_queue(r_class, ENTRY_ACCESSED, re);

  switch (result->op) {
  case CACHE_NUCLEUS_HIT:
    classification |= 1;
    break;
  case CACHE_NUCLEUS_FILTER:
    recency_classifier_push_to_queue(r_class, ENTRY_FILTERED, re);
    break;
  case CACHE_NUCLEUS_REPLACE:
    e = hash_lookup(&r_class->ht, result->old_oblock);
    if (e == NULL) {
      e = alloc_entry(&r_class->entry_pool);
      LOG_ASSERT(e != NULL);
      e->oblock = result->old_oblock;
      hash_insert(&r_class->ht, e);
    }
    evicted_re = to_recency_classifier_entry(e);
    recency_classifier_push_to_queue(r_class, ENTRY_EVICTED, evicted_re);
  case CACHE_NUCLEUS_NEW:
    recency_classifier_push_to_queue(r_class, ENTRY_INSERTED, re);
    break;
  default:
    LOG_FATAL("Unknown cache_nucleus_operation value: %d", result->op);
  }

  ++r_class->count[classification];
}

static void recency_classifier_exit(struct recency_classifier *r_class) {
  hash_exit(&r_class->ht);
  epool_exit(&r_class->entry_pool);
}

static int recency_classifier_init(struct recency_classifier *r_class,
                                   cblock_t cache_size) {
  int r;
  int i;
  cblock_t total_size = 4 * cache_size + 2;

  r = ht_init(&r_class->ht, total_size);
  if (r) {
    LOG_DEBUG("ht_init failed");
    goto ht_init_fail;
  }

  epool_create(&r_class->entry_pool, total_size,
               struct recency_classifier_entry, e);
  if (!r_class->entry_pool.entries_begin || !r_class->entry_pool.entries) {
    LOG_DEBUG("epool_create for entry_pool failed");
    r = -ENOSPC;
    goto entry_pool_create_fail;
  }
  epool_init(&r_class->entry_pool, 0);

  for (i = 0; i < NUM_RECENCY_TYPES; ++i) {
    queue_init(&r_class->q[i]);
    r_class->count[i] = 0;
  }

  r_class->cache_size = cache_size;

  return 0;

entry_pool_create_fail:
  hash_exit(&r_class->ht);
ht_init_fail:
  return r;
}

#endif /* SIM_SIM_RECENCY_CLASSIFIER_H */
