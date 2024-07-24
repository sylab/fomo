/* LIRS caching policy
 *
 * Implementation of the LIRS caching policy using cache_nucleus.
 *
 * Low Inter-reference Recency Set (LIRS) Paper:
 * http://web.cse.ohio-state.edu/hpcs/WWW/HTML/publications/papers/TR-02-6.pdf
 */
#include "lirs_policy.h"
#include "cache_nucleus_internal.h"
#include "common.h"
#include "lirs_policy_struct.h"

#define LIRS_G_FLAG (1 << 0)
#define LIRS_Q_FLAG (1 << 1)
#define LIRS_R_FLAG (1 << 2)
#define LIRS_S_FLAG (1 << 3)

struct lirs_transaction {
  oblock_t oblock;
  bool repeat;
  bool hir_evict;
  bool lir_evict;
  bool is_HIR;
  bool hir_remove;
  bool hir_add;
  bool lir_s_lru;
};

void lirs_transaction_init(struct lirs_policy *lirs,
                           struct lirs_transaction *lt, oblock_t oblock) {
  lt->oblock = oblock;
  lt->repeat = false;
  lt->hir_evict = false;
  lt->lir_evict = false;
  lt->is_HIR = true;
  lt->hir_remove = false;
  lt->hir_add = false;
  lt->lir_s_lru = false;
}

void lirs_transaction_set(struct lirs_policy *lirs,
                          struct lirs_transaction *lt) {
  struct cache_nucleus *cn = &lirs->nucleus;
  struct entry *e = hash_lookup(&cn->ht, lt->oblock);
  struct lirs_entry *le;

  if (lt->oblock == lirs->last_oblock) {
    lt->repeat = true;
    return;
  }

  if (e != NULL) {
    le = to_lirs_entry(e);
    lt->is_HIR = le->is_HIR;
  }

  if (!in_cache(cn, e)) {
    if (policy_cache_is_full(cn)) {
      lt->hir_evict = true;
    } else if (cn->cache_size - policy_residency(cn) > lirs->hir_limit) {
      lt->is_HIR = false;
    }
  } else if (e != NULL && lt->is_HIR) {
    lt->hir_remove = true;
  }

  le = queue_peek_entry(&lirs->s, struct lirs_entry, s_list);
  if (le != NULL && lt->oblock == le->e.oblock) {
    lt->lir_s_lru = true;
  }

  if (e != NULL && lt->is_HIR && in_queue(&lirs->s, &le->s_list)) {
    lt->is_HIR = false;

    if (lirs->lir_count + 1 > cn->cache_size - lirs->hir_limit) {
      lt->lir_evict = true;
    }
  } else if (lt->is_HIR) {
    lt->hir_add = true;
  }
}

bool lirs_transaction_is_legal(struct lirs_policy *lirs,
                               struct lirs_transaction *lt) {
  bool legal = true;

  if (lt->hir_evict) {
    legal &= queue_length(&lirs->q) > 0;
  }

  if (lt->lir_evict) {
    legal &= queue_length(&lirs->s) > 0;
  }

  return legal;
}

void lirs_remove_from_queues(struct lirs_policy *lirs, struct lirs_entry *le) {
  if (in_queue(&lirs->q, &le->q_list) || in_queue(&lirs->g, &le->q_list)) {
    queue_remove(&le->q_list);
  }
  if (in_queue(&lirs->r, &le->r_list)) {
    queue_remove(&le->r_list);
  }
  if (in_queue(&lirs->s, &le->s_list)) {
    queue_remove(&le->s_list);
    lirs->s_len--;
  }
}

void lirs_push_to_queues(struct lirs_policy *lirs, struct lirs_entry *le,
                         int queue_flags) {
  LOG_ASSERT(((queue_flags & LIRS_G_FLAG) && (queue_flags & LIRS_Q_FLAG)) ==
             false);

  if (queue_flags & LIRS_G_FLAG) {
    queue_push(&lirs->g, &le->q_list);
  }
  if (queue_flags & LIRS_Q_FLAG) {
    queue_push(&lirs->q, &le->q_list);
  }
  if (queue_flags & LIRS_R_FLAG) {
    queue_push(&lirs->r, &le->r_list);
  }
  if (queue_flags & LIRS_S_FLAG) {
    queue_push(&lirs->s, &le->s_list);
  }
}

void lirs_remove_entry(struct lirs_policy *lirs, struct lirs_entry *le) {
  struct cache_nucleus *cn = &lirs->nucleus;
  struct entry *e = &le->e;

  lirs_remove_from_queues(lirs, le);

  if (!le->is_HIR) {
    --lirs->lir_count;
  }

  if (in_cache(cn, e) && !e->migrating) {
    inc_demotions(&cn->stats);
  }

  cache_nucleus_remove(cn, e);
}

void lirs_g_evict(struct lirs_policy *lirs) {
  struct lirs_entry *le = queue_peek_entry(&lirs->g, struct lirs_entry, q_list);
  lirs_remove_entry(lirs, le);
}

void lirs_replace_with_meta(struct lirs_policy *lirs,
                            struct lirs_entry *cache_le) {
  struct cache_nucleus *cn = &lirs->nucleus;
  struct entry *cache_e = &cache_le->e;
  oblock_t oblock = cache_e->oblock;
  struct lirs_entry *meta_le;
  struct entry *meta_e;

  LOG_ASSERT(in_queue(&lirs->s, &cache_le->s_list));
  LOG_ASSERT(cache_le->is_HIR);
  LOG_ASSERT(in_cache(cn, cache_e));

  if (meta_is_full(cn)) {
    lirs_g_evict(lirs);
  }

  meta_e = insert_in_meta(cn, oblock);
  meta_le = to_lirs_entry(meta_e);
  meta_le->is_HIR = true;
  lirs_push_to_queues(lirs, meta_le, LIRS_G_FLAG | LIRS_R_FLAG | LIRS_S_FLAG);
  lirs->s_len++;

  queue_swap(&cache_le->r_list, &meta_le->r_list);
  queue_swap(&cache_le->s_list, &meta_le->s_list);

  lirs_remove_entry(lirs, cache_le);
}

void lirs_q_evict(struct lirs_policy *lirs,
                  struct cache_nucleus_result *result) {
  struct lirs_entry *le = queue_peek_entry(&lirs->q, struct lirs_entry, q_list);
  struct entry *e = &le->e;

  result->old_oblock = e->oblock;
  result->dirty_eviction = e->dirty;

  if (in_queue(&lirs->s, &le->s_list)) {
    lirs_replace_with_meta(lirs, le);
  } else {
    lirs_remove_entry(lirs, le);
  }
}

void lirs_r_evict(struct lirs_policy *lirs) {
  struct cache_nucleus *cn = &lirs->nucleus;
  struct lirs_entry *le = queue_peek_entry(&lirs->r, struct lirs_entry, r_list);
  struct entry *e = &le->e;

  queue_remove(&le->s_list);
  queue_remove(&le->r_list);
  lirs->s_len--;

  if (!in_cache(cn, e)) {
    lirs_remove_entry(lirs, le);
  }
}

void lirs_s_evict(struct lirs_policy *lirs) {
  struct cache_nucleus *cn = &lirs->nucleus;
  struct lirs_entry *le = queue_peek_entry(&lirs->s, struct lirs_entry, s_list);
  struct entry *e = &le->e;

  LOG_ASSERT(!le->is_HIR);
  LOG_ASSERT(in_cache(cn, e));

  le->is_HIR = true;
  lirs->lir_count--;

  lirs_remove_from_queues(lirs, le);
  lirs_push_to_queues(lirs, le, LIRS_Q_FLAG);
}

void lirs_find_last_lir_lru(struct lirs_policy *lirs) {
  struct cache_nucleus *cn = &lirs->nucleus;

  while (!queue_empty(&lirs->s)) {
    struct lirs_entry *le =
        queue_peek_entry(&lirs->s, struct lirs_entry, s_list);
    struct entry *e = &le->e;

    if (!le->is_HIR) {
      break;
    }

    LOG_ASSERT(le == queue_peek_entry(&lirs->r, struct lirs_entry, r_list));
    lirs_r_evict(lirs);
  }
}

void lirs_prune_s(struct lirs_policy *lirs) {
  if (lirs->s_len <= lirs->s_maxlen) {
    return;
  }

  lirs_r_evict(lirs);
}

void lirs_transaction_commit(struct lirs_policy *lirs,
                             struct lirs_transaction *lt,
                             struct cache_nucleus_result *result) {
  struct cache_nucleus *cn = &lirs->nucleus;
  struct entry *e = hash_lookup(&cn->ht, lt->oblock);
  struct lirs_entry *le;
  bool in_S = false;
  bool is_HIR = true;
  bool new = e == NULL;
  bool cached = in_cache(cn, e);
  bool meta = in_meta(cn, e);
  bool migrating = false;

  if (e != NULL) {
    le = to_lirs_entry(e);
    in_S = in_queue(&lirs->s, &le->s_list);
    is_HIR = le->is_HIR;
    migrating = e->migrating;
  }

  if (cached) {
    inc_hits(&cn->stats);
    result->op = CACHE_NUCLEUS_HIT;
  } else {
    inc_misses(&cn->stats);
    if (lt->hir_evict) {
      result->op = CACHE_NUCLEUS_REPLACE;
    } else {
      result->op = CACHE_NUCLEUS_NEW;
    }
  }

  if (lt->repeat) {
    return;
  }

  lirs->last_oblock = lt->oblock;

  if (meta) {
    lirs_remove_entry(lirs, le);
  } else if (cached && !migrating) {
    lirs_remove_from_queues(lirs, le);
  } else if (migrating) {
    lirs->s_len--;
  }

  if (lt->lir_s_lru) {
    lirs_find_last_lir_lru(lirs);
  }

  if (lt->hir_evict) {
    lirs_q_evict(lirs, result);
  }

  if (new || meta) {
    e = insert_in_cache(cn, lt->oblock, result);
    le = to_lirs_entry(e);
    le->is_HIR = lt->is_HIR;
    if (!lt->is_HIR) {
      lirs->lir_count++;
    }
    migrating_entry(&cn->cache_pool, e);
  } else if (is_HIR) {
    le->is_HIR = false;
    lirs->lir_count++;
  }

  if (lt->lir_evict) {
    lirs_s_evict(lirs);
    lirs_find_last_lir_lru(lirs);
  }

  if (!e->migrating) {
    if (lt->is_HIR) {
      lirs_push_to_queues(lirs, le, LIRS_Q_FLAG | LIRS_R_FLAG | LIRS_S_FLAG);
    } else {
      lirs_push_to_queues(lirs, le, LIRS_S_FLAG);
    }
  }
  lirs->s_len++;

  lirs_prune_s(lirs);
}

void lirs_remove_api(struct cache_nucleus *cn, oblock_t oblock,
                     bool remove_to_history) {
  struct lirs_policy *lirs = to_lirs_policy(cn);
  struct entry *e = hash_lookup(&cn->ht, oblock);
  struct lirs_entry *le = to_lirs_entry(e);

  if (in_cache(cn, e)) {
    bool HIR_in_S = le->is_HIR && in_queue(&lirs->s, &le->s_list);

    if (remove_to_history && HIR_in_S) {
      lirs_replace_with_meta(lirs, le);
    } else {
      lirs_remove_entry(lirs, le);
    }
  } else {
    lirs_remove_entry(lirs, le);
  }

  // just in case
  lirs_prune_s(lirs);
}

void lirs_insert_api(struct cache_nucleus *cn, oblock_t oblock,
                     cblock_t cblock) {
  struct lirs_policy *lirs = to_lirs_policy(cn);
  struct entry *e = insert_in_cache_at(cn, oblock, cblock, NULL);
  struct lirs_entry *le = to_lirs_entry(e);

  if (cn->cache_size - policy_residency(cn) > lirs->hir_limit) {
    le->is_HIR = false;
    ++lirs->lir_count;
    lirs_push_to_queues(lirs, le, LIRS_S_FLAG);
  } else {
    le->is_HIR = true;
    lirs_push_to_queues(lirs, le, LIRS_Q_FLAG | LIRS_R_FLAG | LIRS_S_FLAG);
  }
  lirs->s_len++;
}

void lirs_migrated_api(struct cache_nucleus *cn, oblock_t oblock) {
  struct lirs_policy *lirs = to_lirs_policy(cn);
  struct entry *e = hash_lookup(&cn->ht, oblock);
  struct lirs_entry *le;

  LOG_ASSERT(e != NULL);
  LOG_ASSERT(e->migrating);

  le = to_lirs_entry(e);
  migrated_entry(&cn->cache_pool, e);

  if (le->is_HIR) {
    lirs_push_to_queues(lirs, le, LIRS_Q_FLAG | LIRS_R_FLAG | LIRS_S_FLAG);
  } else {
    lirs_push_to_queues(lirs, le, LIRS_S_FLAG);
  }

  inc_promotions(&cn->stats);
}

int lirs_map_api(struct cache_nucleus *cn, oblock_t oblock, bool write,
                 struct cache_nucleus_result *result) {
  struct entry *e = hash_lookup(&cn->ht, oblock);
  struct lirs_policy *lirs = to_lirs_policy(cn);
  struct lirs_transaction lt;

  lirs_transaction_init(lirs, &lt, oblock);
  lirs_transaction_set(lirs, &lt);
  if (lirs_transaction_is_legal(lirs, &lt)) {
    lirs_transaction_commit(lirs, &lt, result);

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

void lirs_destroy_api(struct cache_nucleus *cn) {
  struct lirs_policy *lirs = to_lirs_policy(cn);
  epool_exit(&cn->cache_pool);
  epool_exit(&cn->meta_pool);
  policy_exit(cn);
  mem_free(lirs);
}

enum cache_nucleus_operation
lirs_next_victim_api(struct cache_nucleus *cn, oblock_t oblock,
                     struct entry **next_victim_p) {
  struct lirs_policy *lirs = to_lirs_policy(cn);
  struct lirs_transaction lt;

  *next_victim_p = NULL;

  lirs_transaction_init(lirs, &lt, oblock);
  lirs_transaction_set(lirs, &lt);
  if (lirs_transaction_is_legal(lirs, &lt)) {
    if (policy_cache_lookup(cn, oblock) != NULL) {
      return CACHE_NUCLEUS_HIT;
    }
    if (lt.hir_evict) {
      struct lirs_entry *le =
          queue_peek_entry(&lirs->q, struct lirs_entry, q_list);
      *next_victim_p = &le->e;
      return CACHE_NUCLEUS_REPLACE;
    }
    return CACHE_NUCLEUS_NEW;
  }
  return CACHE_NUCLEUS_FAIL;
}

int lirs_init(struct lirs_policy *lirs, cblock_t cache_size,
              cblock_t meta_size) {
  struct cache_nucleus *cn = &lirs->nucleus;
  int i;

  cache_nucleus_init_api(
      cn, lirs_map_api, lirs_remove_api, lirs_insert_api, lirs_migrated_api,
      default_cache_lookup, default_meta_lookup, default_cblock_lookup,
      default_remap, lirs_destroy_api, default_get_stats, default_set_time,
      default_residency, default_cache_is_full, default_infer_cblock,
      lirs_next_victim_api);

  queue_init(&lirs->q);
  queue_init(&lirs->r);
  queue_init(&lirs->s);
  queue_init(&lirs->g);

  // TODO for super small caches do special checks for hir_limit?
  // lirs->lir_limit = cache_size - max(2u, cache_size / 100u);
  lirs->hir_limit = max(2u, cache_size / 100u);

  lirs->lir_count = 0;
  lirs->last_oblock = (oblock_t)-1;

  lirs->s_len = 0;
  lirs->s_maxlen = 2 * cache_size;

  epool_create(&cn->cache_pool, cache_size, struct lirs_entry, e);
  if (!cn->cache_pool.entries_begin || !cn->cache_pool.entries) {
    LOG_DEBUG("epool_create for cache_pool failed.");
    goto cache_epool_create_fail;
  }

  epool_create(&cn->meta_pool, meta_size, struct lirs_entry, e);
  if (!cn->meta_pool.entries_begin || !cn->meta_pool.entries) {
    LOG_DEBUG("epool_create for meta_pool failed.");
    goto meta_epool_create_fail;
  }

  if (policy_init(cn, cache_size, meta_size, CACHE_NUCLEUS_SINGLE_THREAD)) {
    LOG_DEBUG("policy_init failed.");
    goto policy_init_fail;
  }

  return 0;

policy_init_fail:
  epool_exit(&cn->meta_pool);
meta_epool_create_fail:
  epool_exit(&cn->cache_pool);
cache_epool_create_fail:
  return -ENOSPC;
}

struct cache_nucleus *lirs_create(cblock_t cache_size, cblock_t meta_size) {
  int r;
  struct cache_nucleus *cn;
  struct lirs_policy *lirs = mem_alloc(sizeof(*lirs));
  if (!lirs) {
    LOG_DEBUG("mem_alloc failed");
    goto lirs_create_fail;
  }

  cn = &lirs->nucleus;

  r = lirs_init(lirs, cache_size, meta_size);
  if (r) {
    LOG_DEBUG("lirs_init failed. error %d", r);
    goto lirs_init_fail;
  }

  return cn;

lirs_init_fail:
  lirs_destroy_api(cn);
lirs_create_fail:
  return NULL;
}
