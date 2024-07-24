#include "dlirs_policy.h"
#include "cache_nucleus_internal.h"
#include "common.h"
#include "entry.h"
#include "tools/queue.h"

#define DLIRS_F_SCALE 2

struct dlirs_policy {
  struct cache_nucleus nucleus;

  struct queue q;
  struct queue s;
  struct queue g;

  cblock_t cache_size;

  cblock_t lir_limit;
  cblock_t hir_limit;

  cblock_t lir_count;
  cblock_t hir_count;
  cblock_t demoted_count;
  cblock_t nonresident_count;
};

#define DLIRS_Q_FLAG (1 << 0)
#define DLIRS_S_FLAG (1 << 1)
#define DLIRS_G_FLAG (1 << 2)

struct dlirs_entry {
  struct entry e;
  struct queue_head q_list;
  struct queue_head s_list;
  struct queue_head g_list;

  bool is_LIR;
  bool demoted;

  unsigned list_flags;
};

struct dlirs_policy *to_dlirs_policy(struct cache_nucleus *cn) {
  return container_of(cn, struct dlirs_policy, nucleus);
}

struct dlirs_entry *to_dlirs_entry(struct entry *e) {
  return container_of(e, struct dlirs_entry, e);
}

void dlirs_entry_init(struct dlirs_entry *de) {
  queue_head_init(&de->s_list);
  queue_head_init(&de->g_list);
  queue_head_init(&de->q_list);

  de->is_LIR = false;
  de->demoted = false;
}

void dlirs_adjust_size(struct dlirs_policy *dlirs, bool hitHIR) {
  struct cache_nucleus *cn = &dlirs->nucleus;

  if (hitHIR) {
    // NOTE: for translating int((self.dem_count / self.nor_count) + 0.5)
    // due to working with integers we need to figure out better way to round up
    cblock_t round_up =
        ((2 * (dlirs->demoted_count % dlirs->nonresident_count)) >=
         dlirs->nonresident_count)
            ? 1
            : 0;
    dlirs->hir_limit =
        min(cn->cache_size - 1u,
            dlirs->hir_limit +
                max(1u, (dlirs->demoted_count / dlirs->nonresident_count) +
                            round_up));
    dlirs->lir_limit = cn->cache_size - dlirs->hir_limit;
  } else {
    // NOTE: same thing as above, but swapping demoted_count and
    // nonresident_count
    cblock_t round_up =
        ((2 * (dlirs->nonresident_count % dlirs->demoted_count)) >=
         dlirs->demoted_count)
            ? 1
            : 0;
    dlirs->lir_limit =
        min(cn->cache_size - 1u,
            dlirs->lir_limit +
                max(1u, (dlirs->nonresident_count / dlirs->demoted_count) +
                            round_up));
    dlirs->hir_limit = cn->cache_size - dlirs->lir_limit;
  }
}

void dlirs_remove_entry(struct cache_nucleus *cn, struct entry *e) {
  struct dlirs_policy *dlirs = to_dlirs_policy(cn);
  struct dlirs_entry *de = to_dlirs_entry(e);

  if (de->is_LIR) {
    --dlirs->lir_count;
  } else if (in_cache(cn, e)) {
    if (de->demoted) {
      --dlirs->demoted_count;
    }
    --dlirs->hir_count;
  } else {
    --dlirs->nonresident_count;
  }

  if (in_queue(&dlirs->s, &de->s_list)) {
    queue_remove(&de->s_list);
  }
  if (in_queue(&dlirs->g, &de->g_list)) {
    queue_remove(&de->g_list);
  }
  if (in_queue(&dlirs->q, &de->q_list)) {
    queue_remove(&de->q_list);
  }

  cache_nucleus_remove(cn, e);
}

void dlirs_remove_orphan(struct dlirs_policy *dlirs, struct dlirs_entry *de) {
  struct cache_nucleus *cn = &dlirs->nucleus;
  struct entry *e = &de->e;
  if (!in_queue(&dlirs->s, &de->s_list) && !in_queue(&dlirs->g, &de->g_list) &&
      !in_queue(&dlirs->q, &de->q_list)) {
    dlirs_remove_entry(cn, e);
  }
}

struct entry *dlirs_insert_in_cache(struct dlirs_policy *dlirs, oblock_t oblock,
                                    struct cache_nucleus_result *result) {
  struct cache_nucleus *cn = &dlirs->nucleus;
  struct entry *e = insert_in_cache(cn, oblock, result);
  struct dlirs_entry *de = to_dlirs_entry(e);
  dlirs_entry_init(de);
  return e;
}

struct entry *dlirs_insert_in_cache_at(struct dlirs_policy *dlirs,
                                       oblock_t oblock, cblock_t cblock) {
  struct cache_nucleus *cn = &dlirs->nucleus;
  struct entry *e = insert_in_cache_at(cn, oblock, cblock, NULL);
  struct dlirs_entry *de = to_dlirs_entry(e);
  dlirs_entry_init(de);
  return e;
}

struct entry *dlirs_insert_in_meta(struct dlirs_policy *dlirs,
                                   oblock_t oblock) {
  struct cache_nucleus *cn = &dlirs->nucleus;
  struct entry *e = insert_in_meta(cn, oblock);
  struct dlirs_entry *de = to_dlirs_entry(e);
  dlirs_entry_init(de);
  return e;
}

void dlirs_evict_meta_s(struct dlirs_policy *dlirs) {
  struct cache_nucleus *cn = &dlirs->nucleus;
  while (!queue_empty(&dlirs->g)) {
    struct dlirs_entry *de =
        queue_pop_entry(&dlirs->g, struct dlirs_entry, g_list);
    struct entry *e = &de->e;
    queue_remove(&de->s_list);
    if (in_meta(cn, e)) {
      dlirs_remove_entry(cn, e);
      break;
    }
  }
}

struct dlirs_entry *dlirs_replace_with_meta(struct dlirs_policy *dlirs,
                                            struct dlirs_entry *de) {
  struct cache_nucleus *cn = &dlirs->nucleus;
  struct entry *e = &de->e;
  struct dlirs_entry *meta_de = NULL;
  struct entry *meta_copy;
  oblock_t oblock = e->oblock;

  if (cn->meta_size > 0 && in_queue(&dlirs->s, &de->s_list)) {
    ++dlirs->nonresident_count;

    if (meta_is_full(cn)) {
      dlirs_evict_meta_s(dlirs);
    }

    meta_copy = dlirs_insert_in_meta(dlirs, oblock);
    meta_de = to_dlirs_entry(meta_copy);

    queue_swap(&de->s_list, &meta_de->s_list);
    queue_swap(&de->g_list, &meta_de->g_list);
  }
  dlirs_remove_entry(cn, e);

  return meta_de;
}

void dlirs_evict_s(struct dlirs_policy *dlirs) {
  struct dlirs_entry *de =
      queue_pop_entry(&dlirs->s, struct dlirs_entry, s_list);

  queue_remove(&de->s_list);

  if (de->is_LIR) {
    de->is_LIR = false;
    --dlirs->lir_count;

    de->demoted = true;
    ++dlirs->demoted_count;

    queue_push(&dlirs->q, &de->q_list);
    ++dlirs->hir_count;
  } else {
    queue_remove(&de->g_list);
    dlirs_remove_orphan(dlirs, de);
  }
}

// Forward declaration
void dlirs_prune(struct dlirs_policy *dlirs);

void dlirs_evict_lir(struct dlirs_policy *dlirs) {
  struct dlirs_entry *de =
      queue_pop_entry(&dlirs->s, struct dlirs_entry, s_list);

  de->is_LIR = false;
  --dlirs->lir_count;

  de->demoted = true;
  ++dlirs->demoted_count;

  queue_push(&dlirs->q, &de->q_list);
  ++dlirs->hir_count;

  dlirs_prune(dlirs);
}

void dlirs_evict_hir(struct dlirs_policy *dlirs,
                     struct cache_nucleus_result *result) {
  struct cache_nucleus *cn = &dlirs->nucleus;
  struct dlirs_entry *de =
      queue_peek_entry(&dlirs->q, struct dlirs_entry, q_list);
  struct entry *e = &de->e;

  inc_demotions(&cn->stats);

  result->old_oblock = de->e.oblock;
  result->dirty_eviction = de->e.dirty;

  if (in_queue(&dlirs->s, &de->s_list)) {
    dlirs_replace_with_meta(dlirs, de);
  } else {
    dlirs_remove_entry(cn, e);
  }
}

void dlirs_prune(struct dlirs_policy *dlirs) {
  struct cache_nucleus *cn = &dlirs->nucleus;

  while (queue_length(&dlirs->s) > 0) {
    struct dlirs_entry *de =
        queue_peek_entry(&dlirs->s, struct dlirs_entry, s_list);
    if (de->is_LIR) {
      break;
    }
    dlirs_evict_s(dlirs);
  }
}

void dlirs_limit_stack(struct dlirs_policy *dlirs) {
  struct cache_nucleus *cn = &dlirs->nucleus;

  while (dlirs->hir_count + dlirs->lir_count + dlirs->nonresident_count >
         2 * cn->cache_size) {
    struct dlirs_entry *de =
        queue_pop_entry(&dlirs->g, struct dlirs_entry, g_list);
    struct entry *e = &de->e;

    queue_remove(&de->s_list);

    if (in_meta(cn, e)) {
      dlirs_remove_entry(cn, e);
    }
  }
}

void dlirs_miss(struct dlirs_policy *dlirs, oblock_t oblock,
                struct cache_nucleus_result *result) {
  struct cache_nucleus *cn = &dlirs->nucleus;
  struct dlirs_entry *de;
  struct entry *e;

  if (cache_is_full(cn)) {
    result->op = CACHE_NUCLEUS_REPLACE;
  } else {
    result->op = CACHE_NUCLEUS_NEW;
  }

  inc_misses(&cn->stats);
  inc_promotions(&cn->stats);

  if (dlirs->lir_count < dlirs->lir_limit && dlirs->hir_count == 0) {
    e = dlirs_insert_in_cache(dlirs, oblock, result);
    de = to_dlirs_entry(e);

    de->is_LIR = true;
    ++dlirs->lir_count;
    queue_push(&dlirs->s, &de->s_list);
    return;
  }

  if (dlirs->hir_count + dlirs->lir_count >= cn->cache_size) {
    while (dlirs->lir_count > dlirs->lir_limit) {
      dlirs_evict_lir(dlirs);
    }
    dlirs_evict_hir(dlirs, result);
  }

  e = dlirs_insert_in_cache(dlirs, oblock, result);
  de = to_dlirs_entry(e);

  ++dlirs->hir_count;
  queue_push(&dlirs->s, &de->s_list);
  queue_push(&dlirs->g, &de->g_list);
  queue_push(&dlirs->q, &de->q_list);

  dlirs_limit_stack(dlirs);
}

void dlirs_hit_hir_in_q(struct dlirs_policy *dlirs, struct dlirs_entry *de,
                        struct cache_nucleus_result *result) {
  struct cache_nucleus *cn = &dlirs->nucleus;
  struct entry *e = &de->e;

  inc_hits(&cn->stats);

  result->op = CACHE_NUCLEUS_HIT;
  result->cblock = infer_cblock(e);

  if (de->demoted) {
    dlirs_adjust_size(dlirs, false);
    de->demoted = false;
    --dlirs->demoted_count;
  }

  queue_remove(&de->q_list);

  queue_push(&dlirs->q, &de->q_list);
  queue_push(&dlirs->s, &de->s_list);
  queue_push(&dlirs->g, &de->g_list);

  dlirs_limit_stack(dlirs);
}

void dlirs_miss_hir_in_s(struct dlirs_policy *dlirs, struct dlirs_entry *de,
                         struct cache_nucleus_result *result) {
  struct cache_nucleus *cn = &dlirs->nucleus;
  struct entry *e = &de->e;
  oblock_t oblock = e->oblock;

  inc_misses(&cn->stats);
  inc_promotions(&cn->stats);

  if (cache_is_full(cn)) {
    result->op = CACHE_NUCLEUS_REPLACE;
  } else {
    result->op = CACHE_NUCLEUS_NEW;
  }

  dlirs_adjust_size(dlirs, true);

  dlirs_remove_entry(cn, e);

  while (dlirs->lir_count >= dlirs->lir_limit) {
    dlirs_evict_lir(dlirs);
  }
  if (dlirs->hir_count + dlirs->lir_count >= cn->cache_size) {
    dlirs_evict_hir(dlirs, result);
  }

  e = dlirs_insert_in_cache(dlirs, oblock, result);
  de = to_dlirs_entry(e);

  de->is_LIR = true;
  ++dlirs->lir_count;
  queue_push(&dlirs->s, &de->s_list);
}

void dlirs_hit_hir_in_s(struct dlirs_policy *dlirs, struct dlirs_entry *de,
                        struct cache_nucleus_result *result) {
  struct cache_nucleus *cn = &dlirs->nucleus;
  struct entry *e = &de->e;

  inc_hits(&cn->stats);

  result->op = CACHE_NUCLEUS_HIT;
  result->cblock = infer_cblock(e);

  queue_remove(&de->s_list);
  queue_remove(&de->g_list);
  queue_remove(&de->q_list);
  --dlirs->hir_count;

  // we won't be evicting here at all from HIR
  while (dlirs->lir_count >= dlirs->lir_limit) {
    dlirs_evict_lir(dlirs);
  }

  de->is_LIR = true;
  ++dlirs->lir_count;
  queue_push(&dlirs->s, &de->s_list);
}

void dlirs_hit_lir(struct dlirs_policy *dlirs, struct dlirs_entry *de,
                   struct cache_nucleus_result *result) {
  struct cache_nucleus *cn = &dlirs->nucleus;
  struct entry *e = &de->e;
  struct queue_head *first = queue_peek(&dlirs->s);

  inc_hits(&cn->stats);

  result->op = CACHE_NUCLEUS_HIT;
  result->cblock = infer_cblock(e);

  queue_remove(&de->s_list);
  queue_push(&dlirs->s, &de->s_list);

  if (&de->s_list == first) {
    dlirs_prune(dlirs);
  }
}

// cache_nucleus APIs
void dlirs_remove(struct cache_nucleus *cn, oblock_t oblock) {
  struct entry *e = hash_lookup(&cn->ht, oblock);
  if (in_cache(cn, e)) {
    inc_demotions(&cn->stats);
  }
  dlirs_remove_entry(cn, e);
}

void dlirs_remove_preserve(struct cache_nucleus *cn, oblock_t oblock) {
  struct dlirs_policy *dlirs = to_dlirs_policy(cn);
  struct entry *e = hash_lookup(&cn->ht, oblock);
  struct dlirs_entry *de = to_dlirs_entry(e);

  de->list_flags = 0;
  if (in_queue(&dlirs->s, &de->s_list)) {
    de->list_flags |= DLIRS_S_FLAG;
  }
  if (in_queue(&dlirs->g, &de->g_list)) {
    de->list_flags |= DLIRS_G_FLAG;
  }
  if (in_queue(&dlirs->q, &de->q_list)) {
    de->list_flags |= DLIRS_Q_FLAG;
  }

  dlirs_remove_entry(cn, e);

  e->preserved = true;
}

void dlirs_insert(struct cache_nucleus *cn, struct entry *e) {
  struct dlirs_policy *dlirs = to_dlirs_policy(cn);
  struct dlirs_entry *de = to_dlirs_entry(e);
  unsigned list_flags = DLIRS_S_FLAG | DLIRS_Q_FLAG;
  bool is_LIR = false;
  bool demoted = false;

  if (e->preserved) {
    list_flags = de->list_flags;
    is_LIR = de->is_LIR;
    demoted = de->demoted;
    e->preserved = false;
  }

  e = insert_in_cache_at(cn, e->oblock, infer_cblock(e), NULL);
  de = to_dlirs_entry(e);

  if (is_LIR) {
    ++dlirs->lir_count;
  } else {
    if (demoted) {
      ++dlirs->demoted_count;
    }
    ++dlirs->hir_count;
  }

  if (list_flags & DLIRS_S_FLAG) {
    queue_push(&dlirs->s, &de->s_list);
  }
  if (list_flags & DLIRS_G_FLAG) {
    queue_push(&dlirs->g, &de->g_list);
  }
  if (list_flags & DLIRS_Q_FLAG) {
    queue_push(&dlirs->q, &de->q_list);
  }
}

// TODO, double check that no possibility of evicting twice
int dlirs_map(struct cache_nucleus *cn, oblock_t oblock, bool write,
              struct cache_nucleus_result *result) {
  struct entry *e = hash_lookup(&cn->ht, oblock);
  struct dlirs_policy *dlirs = to_dlirs_policy(cn);

  result->op = CACHE_NUCLEUS_MISS;

  inc_ios(&cn->stats);

  if (in_cache(cn, e)) {
    struct dlirs_entry *de = to_dlirs_entry(e);

    if (in_queue(&dlirs->s, &de->s_list)) {
      if (de->is_LIR) {
        dlirs_hit_lir(dlirs, de, result);
      } else {
        dlirs_hit_hir_in_s(dlirs, de, result);
      }
    } else {
      dlirs_hit_hir_in_q(dlirs, de, result);
    }
  } else if (in_meta(cn, e)) {
    struct dlirs_entry *de = to_dlirs_entry(e);
    dlirs_miss_hir_in_s(dlirs, de, result);
  } else {
    dlirs_miss(dlirs, oblock, result);
  }

  // dirty work
  {
    struct entry *cached_entry = hash_lookup(&cn->ht, oblock);
    if (in_cache(cn, cached_entry)) {
      cached_entry->dirty = cached_entry->dirty || write;
    }
  }

  return 0;
}

void dlirs_destroy(struct cache_nucleus *cn) {
  struct dlirs_policy *dlirs = to_dlirs_policy(cn);
  epool_exit(&cn->cache_pool);
  epool_exit(&cn->meta_pool);
  policy_exit(cn);
  mem_free(dlirs);
}

int dlirs_init(struct dlirs_policy *dlirs, cblock_t cache_size,
               cblock_t meta_size) {
  struct cache_nucleus *cn = &dlirs->nucleus;
  int i;

  cache_nucleus_init_api(cn, dlirs_map, dlirs_remove, dlirs_remove_preserve,
                         dlirs_insert, default_lookup, default_cblock_lookup,
                         default_remap, dlirs_destroy, default_get_stats,
                         default_set_time, default_residency,
                         default_cache_is_full);

  queue_init(&dlirs->q);
  queue_init(&dlirs->s);
  queue_init(&dlirs->g);

  dlirs->cache_size = cache_size;

  // TODO for super small caches do special checks for hir_limit?
  dlirs->hir_limit = (cache_size / 100u) + (cache_size % 100u >= 50 ? 1 : 0);
  dlirs->lir_limit = cache_size - max(1u, cache_size / 100u);

  dlirs->hir_count = 0;
  dlirs->lir_count = 0;
  dlirs->demoted_count = 0;
  dlirs->nonresident_count = 0;

  epool_create(&cn->cache_pool, cache_size, struct dlirs_entry, e);
  if (!cn->cache_pool.entries_begin || !cn->cache_pool.entries) {
    LOG_DEBUG("epool_create for cache_pool failed.");
    goto cache_epool_create_fail;
  }

  epool_create(&cn->meta_pool, meta_size, struct dlirs_entry, e);
  if (!cn->meta_pool.entries_begin || !cn->meta_pool.entries) {
    LOG_DEBUG("epool_create for meta_pool failed.");
    goto meta_epool_create_fail;
  }

  if (policy_init(cn, cache_size, meta_size)) {
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

struct cache_nucleus *dlirs_create(cblock_t cache_size, cblock_t meta_size) {
  int r;
  struct cache_nucleus *cn;
  struct dlirs_policy *dlirs = mem_alloc(sizeof(*dlirs));
  if (!dlirs) {
    LOG_DEBUG("mem_alloc failed");
    goto dlirs_create_fail;
  }

  cn = &dlirs->nucleus;

  r = dlirs_init(dlirs, cache_size, meta_size);
  if (r) {
    LOG_DEBUG("dlirs_init failed. error %d", r);
    goto dlirs_init_fail;
  }

  return cn;

dlirs_init_fail:
  dlirs_destroy(cn);
dlirs_create_fail:
  return NULL;
}
