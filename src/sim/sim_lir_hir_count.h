#ifndef SIM_SIM_LIR_HIR_TRACKER_H
#define SIM_SIM_LIR_HIR_TRACKER_H

#include "cache_nucleus.h"
#include "common.h"
#include "entry.h"
#include "tools/hashtable.h"
#include "tools/queue.h"

struct lir_hir_entry {
  struct entry e;
  struct queue_head list;
  unsigned freq;
  bool in_cache;
};

struct lir_hir_tracker {
  struct hashtable ht;
  struct entry_pool entry_pool;
  struct queue q;
  unsigned lir_count;
  unsigned hir_count;
};

static struct lir_hir_entry *to_lir_hir_entry(struct entry *e) {
  return container_of(e, struct lir_hir_entry, e);
}

static void lir_hir_tracker_print_stats(struct lir_hir_tracker *lh_tracker,
                                        unsigned sampling_rate) {
  LOG_PRINT("%u %u", lh_tracker->hir_count * sampling_rate,
            lh_tracker->lir_count * sampling_rate);
}

static void lir_hir_tracker_update(struct lir_hir_tracker *lh_tracker,
                                   oblock_t oblock,
                                   struct cache_nucleus_result *result) {
  struct lir_hir_entry *lhe;
  struct entry *e;
  bool in_last_n = false;

  e = hash_lookup(&lh_tracker->ht, oblock);
  in_last_n = e != NULL;

  if (!in_last_n) {
    if (epool_empty(&lh_tracker->entry_pool)) {
      struct queue_head *evict_qh = queue_pop(&lh_tracker->q);
      struct lir_hir_entry *evict_lhe =
          container_of(evict_qh, struct lir_hir_entry, list);
      struct entry *evict_e = &evict_lhe->e;

      if (evict_lhe->freq == 1) {
        --lh_tracker->hir_count;
      } else if (evict_lhe->freq > 1) {
        --lh_tracker->lir_count;
      }

      evict_lhe->in_cache = false;
      evict_lhe->freq = 0;

      hash_remove(evict_e);
      free_entry(&lh_tracker->entry_pool, evict_e);
    }

    e = alloc_entry(&lh_tracker->entry_pool);
    LOG_ASSERT(e != NULL);
    e->oblock = oblock;
    hash_insert(&lh_tracker->ht, e);
    lhe = to_lir_hir_entry(e);
    lhe->in_cache = false;
    lhe->freq = 0;
  } else {
    lhe = to_lir_hir_entry(e);
    queue_remove(&lhe->list);
  }
  queue_push(&lh_tracker->q, &lhe->list);

  switch (result->op) {
  case CACHE_NUCLEUS_HIT:
    if (!in_last_n) {
      lhe->in_cache = true;
      lhe->freq = 1;
      ++lh_tracker->hir_count;
    } else {
      if (lhe->in_cache) {
        ++lhe->freq;
        if (lhe->freq == 2) {
          --lh_tracker->hir_count;
          ++lh_tracker->lir_count;
        }
      }
    }
    break;
  case CACHE_NUCLEUS_FILTER:
  case CACHE_NUCLEUS_FAIL:
    lhe->in_cache = false;
    lhe->freq = 0;
    break;
  case CACHE_NUCLEUS_REPLACE: {
    struct entry *evict_e = hash_lookup(&lh_tracker->ht, result->old_oblock);
    if (evict_e != NULL) {
      struct lir_hir_entry *evict_lhe = to_lir_hir_entry(evict_e);
      evict_lhe->in_cache = false;

      if (evict_lhe->freq == 1) {
        --lh_tracker->hir_count;
      } else if (evict_lhe->freq > 1) {
        --lh_tracker->lir_count;
      }

      evict_lhe->freq = 0;
    }
  }
  case CACHE_NUCLEUS_NEW:
    lhe->in_cache = true;
    lhe->freq = 1;
    ++lh_tracker->hir_count;
    break;
  }
}

static void lir_hir_tracker_exit(struct lir_hir_tracker *lh_tracker) {
  hash_exit(&lh_tracker->ht);
  epool_exit(&lh_tracker->entry_pool);
}

static int lir_hir_tracker_init(struct lir_hir_tracker *lh_tracker,
                                cblock_t cache_size) {
  int r;

  r = ht_init(&lh_tracker->ht, cache_size);
  if (r) {
    LOG_DEBUG("ht_init failed");
    goto ht_init_fail;
  }

  epool_create(&lh_tracker->entry_pool, cache_size, struct lir_hir_entry, e);
  if (!lh_tracker->entry_pool.entries_begin ||
      !lh_tracker->entry_pool.entries) {
    LOG_DEBUG("epool_create for entry_pool failed");
    r = -ENOSPC;
    goto entry_pool_create_fail;
  }
  epool_init(&lh_tracker->entry_pool, 0);

  queue_init(&lh_tracker->q);

  lh_tracker->lir_count = 0;
  lh_tracker->hir_count = 0;

  return 0;

entry_pool_create_fail:
  hash_exit(&lh_tracker->ht);
ht_init_fail:
  return r;
}

#endif /* SIM_SIM_LIR_HIR_TRACKER_H */
