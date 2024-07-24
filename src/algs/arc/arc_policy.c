/* Adaptive Replacement Cache (ARC)
 *
 *
 * Link to paper:
 * https://dbs.uni-leipzig.de/file/ARC.pdf
 */

#include "arc_policy.h"
#include "arc_policy_struct.h"
#include "cache_nucleus_internal.h"
#include "common.h"
#include "tools/queue.h"

struct arc_transaction {
  oblock_t oblock;
  cblock_t p;
  enum arc_list current_loc;
  enum arc_list dest;
  enum arc_list cache_evict;
  bool evict_T1_to_B1;
  enum arc_list meta_evict;
};

void arc_transaction_init(struct arc_policy *arc, struct arc_transaction *at,
                          oblock_t oblock) {
  at->oblock = oblock;
  at->p = arc->p;
  at->current_loc = ARC_NULL;
  at->dest = ARC_NULL;
  at->cache_evict = ARC_NULL;
  at->evict_T1_to_B1 = false;
  at->meta_evict = ARC_NULL;
}

void arc_replace(struct arc_policy *arc, struct arc_transaction *at) {
  cblock_t T1_len = arc->length[ARC_T1];

  if (T1_len >= 1 &&
      ((at->current_loc == ARC_B2 && T1_len == at->p) || T1_len > at->p)) {
    at->cache_evict = ARC_T1;
    at->evict_T1_to_B1 = true;
  } else {
    at->cache_evict = ARC_T2;
  }
}

void arc_transaction_set(struct arc_policy *arc, struct arc_transaction *at) {
  struct cache_nucleus *cn = &arc->nucleus;
  struct entry *e = hash_lookup(&cn->ht, at->oblock);
  cblock_t cache_size = cn->cache_size;
  cblock_t T1_len = arc->length[ARC_T1];
  cblock_t T2_len = arc->length[ARC_T2];
  cblock_t B1_len = arc->length[ARC_B1];
  cblock_t B2_len = arc->length[ARC_B2];
  cblock_t L1_len = T1_len + B1_len;
  cblock_t L2_len = T2_len + B2_len;

  if (e != NULL) {
    struct arc_entry *ae = to_arc_entry(e);
    at->current_loc = ae->loc;

    if (in_cache(cn, e)) {
      at->dest = ARC_T2;
    } else if (ae->loc == ARC_B1) {
      at->p = min(cache_size, arc->p + max(B2_len / B1_len, 1u));
      arc_replace(arc, at);
      at->dest = ARC_T2;
    } else {
      /* NOTE: avoid underflow with subtraction */
      uint64_t tmp = max(B1_len / B2_len, 1u);
      if (arc->p > tmp) {
        at->p = arc->p - tmp;
      } else {
        at->p = 0u;
      }
      arc_replace(arc, at);
      at->dest = ARC_T2;
    }
  } else {
    if (L1_len == cache_size) {
      if (T1_len < cache_size) {
        at->meta_evict = ARC_B1;
        arc_replace(arc, at);
      } else {
        at->cache_evict = ARC_T1;
      }
    } else if (L1_len < cache_size && L1_len + L2_len >= cache_size) {
      if (L1_len + L2_len == 2 * cache_size) {
        at->meta_evict = ARC_B2;
      }
      arc_replace(arc, at);
    }
    at->dest = ARC_T1;
  }
}

bool arc_transaction_is_legal(struct arc_policy *arc,
                              struct arc_transaction *at) {
  struct cache_nucleus *cn = &arc->nucleus;
  bool legal = true;

  if (at->cache_evict != ARC_NULL) {
    legal &= queue_length(&arc->arc_q[at->cache_evict]) > 0;
  }

  return legal;
}

void arc_remove_entry(struct arc_policy *arc, struct arc_entry *ae) {
  struct cache_nucleus *cn = &arc->nucleus;
  struct entry *e = &ae->e;

  --arc->length[ae->loc];
  if (!e->migrating) {
    queue_remove(&ae->arc_list);
    if (in_cache(cn, e)) {
      inc_demotions(&cn->stats);
    }
  }
  ae->loc = ARC_NULL;

  cache_nucleus_remove(cn, e);
}

void arc_evict(struct arc_policy *arc, enum arc_list list,
               struct cache_nucleus_result *result) {
  struct cache_nucleus *cn = &arc->nucleus;
  struct arc_entry *ae =
      queue_pop_entry(&arc->arc_q[list], struct arc_entry, arc_list);
  struct entry *evicted = &ae->e;
  if (result != NULL) {
    result->old_oblock = evicted->oblock;
    result->dirty_eviction = evicted->dirty;
  }
  arc_remove_entry(arc, ae);
}

void arc_set_for_list(struct arc_policy *arc, struct arc_entry *ae,
                      enum arc_list list) {
  if (ae->loc != ARC_NULL) {
    --arc->length[ae->loc];
  }

  ae->loc = list;
  ++arc->length[list];
}

void arc_transaction_commit(struct arc_policy *arc, struct arc_transaction *at,
                            struct cache_nucleus_result *result) {
  struct cache_nucleus *cn = &arc->nucleus;
  struct entry *e = hash_lookup(&cn->ht, at->oblock);
  struct arc_entry *ae;
  bool hit = at->current_loc == ARC_T1 || at->current_loc == ARC_T2;
  bool migrating = false;

  arc->p = at->p;

  if (e != NULL) {
    migrating = e->migrating;
  }

  if (hit) {
    result->op = CACHE_NUCLEUS_HIT;
    inc_hits(&cn->stats);
  } else {
    inc_misses(&cn->stats);
    if (at->cache_evict != ARC_NULL) {
      result->op = CACHE_NUCLEUS_REPLACE;
    } else {
      result->op = CACHE_NUCLEUS_NEW;
    }
  }

  /* remove from metadata ahead of time */
  if (e != NULL && !hit) {
    ae = to_arc_entry(e);
    arc_remove_entry(arc, ae);
  }

  if (at->meta_evict != ARC_NULL) {
    arc_evict(arc, at->meta_evict, NULL);
  }

  if (at->cache_evict != ARC_NULL) {
    arc_evict(arc, at->cache_evict, result);

    if (at->evict_T1_to_B1 || at->cache_evict == ARC_T2) {
      struct entry *meta_e = insert_in_meta(cn, result->old_oblock);
      struct arc_entry *meta_ae = to_arc_entry(meta_e);

      enum arc_list meta_dest = ARC_B1;
      if (at->cache_evict == ARC_T2) {
        meta_dest = ARC_B2;
      }

      meta_ae->loc = ARC_NULL;
      arc_set_for_list(arc, meta_ae, meta_dest);
      queue_push(&arc->arc_q[meta_dest], &meta_ae->arc_list);
    }
  }

  if (hit) {
    ae = to_arc_entry(e);
    arc_set_for_list(arc, ae, at->dest);

    result->cblock = infer_cblock(&cn->cache_pool, e);

    if (!migrating) {
      queue_remove(&ae->arc_list);
      queue_push(&arc->arc_q[at->dest], &ae->arc_list);
    }
  } else {
    e = insert_in_cache(cn, at->oblock, result);
    ae = to_arc_entry(e);

    ae->loc = ARC_NULL;
    arc_set_for_list(arc, ae, at->dest);
    migrating_entry(&cn->cache_pool, e);
  }
}

void arc_remove_api(struct cache_nucleus *cn, oblock_t oblock,
                    bool remove_to_history) {
  struct arc_policy *arc = to_arc_policy(cn);
  struct entry *e = hash_lookup(&cn->ht, oblock);
  struct arc_entry *ae = to_arc_entry(e);
  enum arc_list loc = ae->loc;
  bool e_in_cache = in_cache(cn, e);

  arc_remove_entry(arc, ae);

  if (e_in_cache && remove_to_history) {
    enum arc_list dest = ARC_NULL;

    if (loc == ARC_T1) {
      dest = ARC_B1;
    } else {
      dest = ARC_B2;
    }

    if (meta_is_full(cn)) {
      arc_evict(arc, dest, NULL);
    }

    e = insert_in_meta(cn, oblock);
    ae = to_arc_entry(e);

    ae->loc = ARC_NULL;
    arc_set_for_list(arc, ae, dest);
    queue_push(&arc->arc_q[dest], &ae->arc_list);
  }
}

void arc_insert_api(struct cache_nucleus *cn, oblock_t oblock,
                    cblock_t cblock) {
  struct arc_policy *arc = to_arc_policy(cn);
  struct entry *e = insert_in_cache_at(cn, oblock, cblock, NULL);
  struct arc_entry *ae = to_arc_entry(e);

  ae->loc = ARC_NULL;
  arc_set_for_list(arc, ae, ARC_T1);
  queue_push(&arc->arc_q[ARC_T1], &ae->arc_list);
}

void arc_migrated_api(struct cache_nucleus *cn, oblock_t oblock) {
  struct arc_policy *arc = to_arc_policy(cn);
  struct entry *e = hash_lookup(&cn->ht, oblock);
  struct arc_entry *ae;

  LOG_ASSERT(e != NULL);
  LOG_ASSERT(e->migrating);

  ae = to_arc_entry(e);
  migrated_entry(&cn->cache_pool, e);

  LOG_ASSERT(ae->loc != ARC_NULL);

  queue_push(&arc->arc_q[ae->loc], &ae->arc_list);

  inc_promotions(&cn->stats);
}

int arc_map_api(struct cache_nucleus *cn, oblock_t oblock, bool write,
                struct cache_nucleus_result *result) {
  struct arc_policy *arc = to_arc_policy(cn);
  struct entry *e = hash_lookup(&cn->ht, oblock);
  struct arc_transaction at;

  arc_transaction_init(arc, &at, oblock);
  arc_transaction_set(arc, &at);
  if (arc_transaction_is_legal(arc, &at)) {
    arc_transaction_commit(arc, &at, result);

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

void arc_destroy_api(struct cache_nucleus *cn) {
  struct arc_policy *arc = to_arc_policy(cn);
  epool_exit(&cn->cache_pool);
  epool_exit(&cn->meta_pool);
  policy_exit(cn);
  mem_free(arc);
}

enum cache_nucleus_operation arc_next_victim_api(struct cache_nucleus *cn,
                                                 oblock_t oblock,
                                                 struct entry **next_victim_p) {
  struct arc_policy *arc = to_arc_policy(cn);
  struct arc_transaction at;

  *next_victim_p = NULL;

  arc_transaction_init(arc, &at, oblock);
  arc_transaction_set(arc, &at);

  if (arc_transaction_is_legal(arc, &at)) {
    if (at.cache_evict != ARC_NULL) {
    }

    if (at.current_loc == ARC_T1 || at.current_loc == ARC_T2) {
      return CACHE_NUCLEUS_HIT;
    } else if (at.cache_evict != ARC_NULL) {
      struct arc_entry *ae = queue_peek_entry(&arc->arc_q[at.cache_evict],
                                              struct arc_entry, arc_list);
      *next_victim_p = &ae->e;
      return CACHE_NUCLEUS_REPLACE;
    } else {
      return CACHE_NUCLEUS_NEW;
    }
  }

  return CACHE_NUCLEUS_FAIL;
}

int arc_init(struct arc_policy *arc, cblock_t cache_size, cblock_t meta_size) {
  struct cache_nucleus *cn = &arc->nucleus;
  int i;

  cache_nucleus_init_api(
      cn, arc_map_api, arc_remove_api, arc_insert_api, arc_migrated_api,
      default_cache_lookup, default_meta_lookup, default_cblock_lookup,
      default_remap, arc_destroy_api, default_get_stats, default_set_time,
      default_residency, default_cache_is_full, default_infer_cblock,
      arc_next_victim_api);

  for (i = 0; i < NUM_ARC_LISTS; ++i) {
    queue_init(&arc->arc_q[i]);
    arc->length[i] = 0;
  }

  arc->p = 0;

  epool_create(&cn->cache_pool, cache_size, struct arc_entry, e);
  if (!cn->cache_pool.entries_begin || !cn->cache_pool.entries) {
    LOG_DEBUG("epool_create for cache_pool failed.");
    goto cache_epool_create_fail;
  }

  epool_create(&cn->meta_pool, meta_size, struct arc_entry, e);
  if (!cn->meta_pool.entries_begin || !cn->meta_pool.entries) {
    LOG_DEBUG("epool_create for meta_pool failed.");
    goto meta_epool_create_fail;
  }

  if (policy_init(cn, cache_size, meta_size, CACHE_NUCLEUS_SINGLE_THREAD)) {
    LOG_DEBUG("policy_init failed");
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

struct cache_nucleus *arc_create(cblock_t cache_size, cblock_t meta_size) {
  int r;
  struct cache_nucleus *cn;
  struct arc_policy *arc = mem_alloc(sizeof(*arc));
  if (!arc) {
    LOG_DEBUG("mem_alloc failed");
    goto arc_create_fail;
  }

  cn = &arc->nucleus;

  r = arc_init(arc, cache_size, meta_size);
  if (r) {
    LOG_DEBUG("arc_init failed. error %d", r);
    goto arc_init_fail;
  }

  return cn;

arc_init_fail:
  arc_destroy_api(cn);
arc_create_fail:
  return NULL;
}
