#ifndef SIM_SIM_HOARD_H
#define SIM_SIM_HOARD_H

#include "cache_nucleus.h"
#include "common.h"
#include "entry.h"
#include "tools/hashtable.h"
#include "tools/queue.h"

struct hoard_tracker_entry {
  struct entry e;
  struct queue_head list;
  unsigned freq;
  bool in_cache;
};

struct hoard_tracker {
  struct hashtable ht;
  struct entry_pool entry_pool;
  struct queue q;
  unsigned count;
  cblock_t cache_size;
};

static struct hoard_tracker_entry *to_hoard_tracker_entry(struct entry *e) {
  return container_of(e, struct hoard_tracker_entry, e);
}

static void hoard_tracker_print_stats(struct hoard_tracker *h_tracker,
                                      unsigned sampling_rate) {
  LOG_PRINT("%u", h_tracker->count * sampling_rate);
}

static void hoard_tracker_update(struct hoard_tracker *h_tracker,
                                 oblock_t oblock,
                                 struct cache_nucleus_result *result) {
  struct hoard_tracker_entry *he;
  struct entry *e;
  struct queue *q = &h_tracker->q;

  e = hash_lookup(&h_tracker->ht, oblock);

  // definition:
  // the percentage of entries in cache that have been accessed at least twice
  // in the past since entering the cache, but is not among the last 2N unique
  // pages accessed

  switch (result->op) {
  case CACHE_NUCLEUS_HIT:
    LOG_ASSERT(e != NULL);
    he = to_hoard_tracker_entry(e);
    LOG_ASSERT(he->in_cache);
    if (!in_queue(q, &he->list)) {
      if (he->freq > 1) {
        --h_tracker->count;
      }
    } else {
      queue_remove(&he->list);
    }
    queue_push(&h_tracker->q, &he->list);
    ++he->freq;
    break;
  case CACHE_NUCLEUS_FILTER:
  case CACHE_NUCLEUS_FAIL:
    if (e == NULL) {
      e = alloc_entry(&h_tracker->entry_pool);
      LOG_ASSERT(e != NULL);
      he = to_hoard_tracker_entry(e);
      e->oblock = oblock;
      he->in_cache = false;
      he->freq = 0;
      hash_insert(&h_tracker->ht, e);
    } else {
      he = to_hoard_tracker_entry(e);
      if (in_queue(q, &he->list)) {
        queue_remove(&he->list);
      }
    }
    queue_push(q, &he->list);
    break;
  case CACHE_NUCLEUS_REPLACE:
    // mark evicted not in cache, free entry if not in queue
    {
      struct entry *evicted = hash_lookup(&h_tracker->ht, result->old_oblock);
      struct hoard_tracker_entry *evicted_he;
      LOG_ASSERT(evicted != NULL);
      evicted_he = to_hoard_tracker_entry(evicted);
      if (!in_queue(q, &evicted_he->list)) {
        if (evicted_he->freq > 1) {
          --h_tracker->count;
        }
        hash_remove(evicted);
        free_entry(&h_tracker->entry_pool, evicted);
      } else {
        evicted_he->in_cache = false;
        evicted_he->freq = 0;
      }
    }
  case CACHE_NUCLEUS_NEW:
    if (e != NULL) {
      he = to_hoard_tracker_entry(e);
      LOG_ASSERT(in_queue(q, &he->list));
      queue_remove(&he->list);
    } else {
      e = alloc_entry(&h_tracker->entry_pool);
      he = to_hoard_tracker_entry(e);
      e->oblock = oblock;
      hash_insert(&h_tracker->ht, e);
    }
    he->freq = 1;
    he->in_cache = true;
    queue_push(q, &he->list);
    break;
  }

  // evict from queue
  if (queue_length(q) > 2 * h_tracker->cache_size) {
    struct queue_head *e_qh = queue_pop(q);
    struct hoard_tracker_entry *e_he =
        container_of(e_qh, struct hoard_tracker_entry, list);
    if (e_he->in_cache) {
      if (e_he->freq > 1) {
        ++h_tracker->count;
      }
    } else {
      hash_remove(&e_he->e);
      free_entry(&h_tracker->entry_pool, &e_he->e);
    }
  }
}

static void hoard_tracker_exit(struct hoard_tracker *h_tracker) {
  hash_exit(&h_tracker->ht);
  epool_exit(&h_tracker->entry_pool);
}

static int hoard_tracker_init(struct hoard_tracker *h_tracker,
                              cblock_t cache_size) {
  int r;
  r = ht_init(&h_tracker->ht, 2 * cache_size);
  if (r) {
    LOG_DEBUG("ht_init failed");
    goto ht_init_fail;
  }

  epool_create(&h_tracker->entry_pool, 3 * cache_size + 1,
               struct hoard_tracker_entry, e);
  if (!h_tracker->entry_pool.entries_begin || !h_tracker->entry_pool.entries) {
    LOG_DEBUG("epool_create for cache_pool failed");
    r = -ENOSPC;
    goto cache_epool_create_fail;
  }
  epool_init(&h_tracker->entry_pool, 0);

  queue_init(&h_tracker->q);

  h_tracker->count = 0;

  h_tracker->cache_size = cache_size;

  return 0;

cache_epool_create_fail:
  hash_exit(&h_tracker->ht);
ht_init_fail:
  return r;
}

#endif /* SIM_SIM_HOARD_H */
