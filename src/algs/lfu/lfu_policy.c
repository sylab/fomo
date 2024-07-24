/* LFU caching policy
 *
 * Implementation of the LFU caching policy using cache_nucleus.
 * This version of LFU is simple and does not use any aging methods.
 *
 * Least Frequently Used (LFU) Paper:
 *
 */

#include "lfu_policy.h"
#include "cache_nucleus_internal.h"
#include "common.h"
#include "entry.h"
#include "lfu_policy_struct.h"
#include "tools/heap.h"

int lfu_compare(struct heap_head *hh_a, struct heap_head *hh_b) {
  struct lfu_entry *le_a = container_of(hh_a, struct lfu_entry, lfu_list);
  struct lfu_entry *le_b = container_of(hh_b, struct lfu_entry, lfu_list);

  if (hh_a->key == hh_b->key) {
    return le_a->e.time - le_b->e.time;
  }

  return hh_a->key - hh_b->key;
}

struct lfu_transaction {
  oblock_t oblock;
  bool hit;
  bool evict;
};

void lfu_transaction_init(struct lfu_policy *lfu, struct lfu_transaction *lt,
                          oblock_t oblock) {
  lt->oblock = oblock;
  lt->hit = false;
  lt->evict = false;
}

void lfu_transaction_set(struct lfu_policy *lfu, struct lfu_transaction *lt) {
  struct cache_nucleus *cn = &lfu->nucleus;

  if (hash_lookup(&cn->ht, lt->oblock) != NULL) {
    lt->hit = true;
  } else if (cache_is_full(cn)) {
    lt->evict = true;
  }
}

bool lfu_transaction_is_legal(struct lfu_policy *lfu,
                              struct lfu_transaction *lt) {
  bool legal = true;
  if (lt->evict) {
    legal &= lfu->heap.size > 0;
  }
  return legal;
}

void lfu_remove_entry(struct lfu_policy *lfu, struct lfu_entry *le) {
  struct cache_nucleus *cn = &lfu->nucleus;
  struct entry *e = &le->e;
  if (!e->migrating) {
    heap_delete(&lfu->heap, &le->lfu_list);
    inc_demotions(&cn->stats);
  }
  cache_nucleus_remove(cn, e);
}

void lfu_transaction_commit(struct lfu_policy *lfu, struct lfu_transaction *lt,
                            struct cache_nucleus_result *result) {
  struct cache_nucleus *cn = &lfu->nucleus;

  result->op = CACHE_NUCLEUS_NEW;

  if (lt->evict) {
    struct lfu_entry *demoted =
        heap_min_entry(&lfu->heap, struct lfu_entry, lfu_list);
    struct entry *e = &demoted->e;

    result->old_oblock = e->oblock;
    result->dirty_eviction = e->dirty;
    lfu_remove_entry(lfu, demoted);

    result->op = CACHE_NUCLEUS_REPLACE;
  }

  if (lt->hit) {
    struct entry *e = hash_lookup(&cn->ht, lt->oblock);
    struct lfu_entry *le = to_lfu_entry(e);

    result->cblock = infer_cblock(&cn->cache_pool, e);

    e->time = cn->time;
    ++(le->lfu_list.key);

    if (!e->migrating) {
      heapify(&lfu->heap, le->lfu_list.index);
    }

    result->op = CACHE_NUCLEUS_HIT;
    inc_hits(&cn->stats);
  } else {
    struct entry *e = insert_in_cache(cn, lt->oblock, result);
    struct lfu_entry *le = to_lfu_entry(e);

    e->time = cn->time;
    le->lfu_list.key = 1;
    migrating_entry(&cn->cache_pool, e);

    inc_misses(&cn->stats);
  }
}

void lfu_remove_api(struct cache_nucleus *cn, oblock_t oblock,
                    bool remove_to_history) {
  struct lfu_policy *lfu = to_lfu_policy(cn);
  struct entry *e = hash_lookup(&cn->ht, oblock);
  struct lfu_entry *le = to_lfu_entry(e);
  lfu_remove_entry(lfu, le);
}

void lfu_insert_api(struct cache_nucleus *cn, oblock_t oblock,
                    cblock_t cblock) {
  struct lfu_policy *lfu = to_lfu_policy(cn);
  struct entry *e = insert_in_cache_at(cn, oblock, cblock, NULL);
  struct lfu_entry *le = to_lfu_entry(e);

  e->time = cn->time;
  le->lfu_list.key = 1;
  heap_insert(&lfu->heap, &le->lfu_list, 1);
}

void lfu_migrated_api(struct cache_nucleus *cn, oblock_t oblock) {
  struct lfu_policy *lfu = to_lfu_policy(cn);
  struct entry *e = hash_lookup(&cn->ht, oblock);
  struct lfu_entry *le = to_lfu_entry(e);

  migrated_entry(&cn->cache_pool, e);
  heap_insert(&lfu->heap, &le->lfu_list, le->lfu_list.key);

  inc_promotions(&cn->stats);
}

int lfu_map_api(struct cache_nucleus *cn, oblock_t oblock, bool write,
                struct cache_nucleus_result *result) {
  struct entry *e = hash_lookup(&cn->ht, oblock);
  struct lfu_policy *lfu = to_lfu_policy(cn);
  struct lfu_transaction lt;

  lfu_transaction_init(lfu, &lt, oblock);
  lfu_transaction_set(lfu, &lt);
  if (lfu_transaction_is_legal(lfu, &lt)) {
    lfu_transaction_commit(lfu, &lt, result);

    inc_ios(&cn->stats);

    // dirty work
    {
      struct entry *cached_entry = hash_lookup(&cn->ht, oblock);
      if (in_cache(cn, cached_entry)) {
        cached_entry->dirty = cached_entry->dirty || write;
      }
    }
  } else {
    result->op = CACHE_NUCLEUS_FAIL;
  }

  return 0;
}

void lfu_destroy_api(struct cache_nucleus *cn) {
  struct lfu_policy *lfu = to_lfu_policy(cn);

  heap_exit(&lfu->heap);
  epool_exit(&cn->cache_pool);
  policy_exit(cn);
  mem_free(lfu);
}

enum cache_nucleus_operation lfu_next_victim_api(struct cache_nucleus *cn,
                                                 oblock_t oblock,
                                                 struct entry **next_victim_p) {
  struct lfu_policy *lfu = to_lfu_policy(cn);
  struct lfu_transaction lt;

  lt.oblock = oblock;
  *next_victim_p = NULL;

  lfu_transaction_set(lfu, &lt);
  if (lfu_transaction_is_legal(lfu, &lt)) {
    if (lt.evict) {
      struct lfu_entry *le =
          heap_min_entry(&lfu->heap, struct lfu_entry, lfu_list);
      *next_victim_p = &le->e;
      return CACHE_NUCLEUS_REPLACE;
    }
    if (lt.hit) {
      return CACHE_NUCLEUS_HIT;
    }
    return CACHE_NUCLEUS_NEW;
  }
  return CACHE_NUCLEUS_FAIL;
}

int lfu_init(struct lfu_policy *lfu, cblock_t cache_size) {
  int i;
  struct cache_nucleus *cn = &lfu->nucleus;

  cache_nucleus_init_api(
      cn, lfu_map_api, lfu_remove_api, lfu_insert_api, lfu_migrated_api,
      default_cache_lookup, default_meta_lookup, default_cblock_lookup,
      default_remap, lfu_destroy_api, default_get_stats, default_set_time,
      default_residency, default_cache_is_full, default_infer_cblock,
      lfu_next_victim_api);

  epool_create(&cn->cache_pool, cache_size, struct lfu_entry, e);
  if (!cn->cache_pool.entries_begin || !cn->cache_pool.entries) {
    LOG_DEBUG("epool_create for cache_pool failed.");
    goto cache_epool_create_fail;
  }

  if (heap_init(&lfu->heap, cache_size)) {
    LOG_DEBUG("heap_init failed for heap.");
    goto heap_init_fail;
  }

  heap_set_compare(&lfu->heap, lfu_compare);

  if (policy_init(cn, cache_size, 0, CACHE_NUCLEUS_SINGLE_THREAD)) {
    LOG_DEBUG("policy_init failed");
    goto policy_init_fail;
  }

  return 0;

policy_init_fail:
  heap_exit(&lfu->heap);
heap_init_fail:
  epool_exit(&cn->cache_pool);
cache_epool_create_fail:
  return -ENOSPC;
}

struct cache_nucleus *lfu_create(cblock_t cache_size, cblock_t meta_size) {
  int r;
  struct cache_nucleus *cn;
  struct lfu_policy *lfu = mem_alloc(sizeof(*lfu));
  if (lfu == NULL) {
    LOG_DEBUG("mem_alloc failed");
    goto lfu_create_fail;
  }

  cn = &lfu->nucleus;

  r = lfu_init(lfu, cache_size);
  if (r) {
    LOG_DEBUG("lfu_init failed. error %d", r);
    goto lfu_init_fail;
  }

  return cn;

lfu_init_fail:
  lfu_destroy_api(cn);
lfu_create_fail:
  return NULL;
}
