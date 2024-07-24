/* Lazy Adaptive Replacement Cache
 *
 * Link to paper:
 * http://storageconference.us/2013/Papers/2013.Paper.28.pdf
 */

#include "larc_policy.h"
#include "cache_nucleus_internal.h"
#include "common.h"
#include "larc_policy_struct.h"

enum larc_list { LARC_Q, LARC_G, LARC_NULL };

struct larc_transaction {
  oblock_t oblock;
  cblock_t g_size;
  bool c_hit;
  bool c_evict;
  bool g_hit;
  bool g_evict;
  enum larc_list dest;
};

void larc_transaction_init(struct larc_policy *larc,
                           struct larc_transaction *lt, oblock_t oblock) {
  lt->oblock = oblock;
  lt->g_size = larc->g_size;
  lt->c_hit = false;
  lt->c_evict = false;
  lt->g_hit = false;
  lt->g_evict = false;
  lt->dest = LARC_NULL;
}

cblock_t larc_increase_g_size(struct larc_policy *larc) {
  struct cache_nucleus *cn = &larc->nucleus;
  cblock_t c_size = cn->meta_size;
  cblock_t g_size = larc->g_size;

  return min((9 * c_size) / 10, g_size + (c_size / g_size));
}

cblock_t larc_decrease_g_size(struct larc_policy *larc) {
  struct cache_nucleus *cn = &larc->nucleus;
  cblock_t c_size = cn->meta_size;
  cblock_t g_size = larc->g_size;
  // use tmp to protect against underflow
  cblock_t tmp = c_size / (c_size - g_size);
  if (tmp < g_size) {
    return max(c_size / 10, g_size - tmp);
  } else {
    return c_size / 10;
  }
}

void larc_transaction_set(struct larc_policy *larc,
                          struct larc_transaction *lt) {
  struct cache_nucleus *cn = &larc->nucleus;
  struct entry *e = hash_lookup(&cn->ht, lt->oblock);

  /*
   * C  = Cache size
   * Cr = Ghost size
   * Q  = Cache Queue
   * Qr = Ghost Queue
   *
   * Initalize: Cr = 0.1 * C
   *
   * access(B):
   *   if B is in Q then
   *     move B to the MRU end of Q
   *     Cr = max(0.1 * C, Cr - (C / (C - Cr)))
   *     redirect the request to the corresponding SSD block
   *     return
   *   end if
   *
   *   Cr = min(0.9 * C, Cr + (C / Cr))
   *   if B is in Qr then
   *     remove B from Qr
   *     insert B to the MRU end of Q
   *     if |Q| > C then
   *       remove the LRU block D in Q
   *       allocate the SSD block of D to B
   *     else
   *       allocate a free SSD block to B
   *     end if
   *   else
   *     insert B to the MRU end of Qr
   *     if |Qr| > Cr then
   *       remove the LRU block of Qr
   *     end if
   *   end if
   */
  if (e != NULL && in_cache(cn, e)) {
    lt->c_hit = true;
    lt->g_size = larc_decrease_g_size(larc);
    lt->dest = LARC_Q;
  } else {
    lt->g_size = larc_increase_g_size(larc);

    if (e != NULL && in_meta(cn, e)) {
      lt->g_hit = true;
      lt->dest = LARC_Q;
      if (cache_is_full(cn)) {
        lt->c_evict = true;
      }
      // NOTE: temp until decision on keeping or removing caching when not full
    } else if (!cache_is_full(cn)) {
      lt->dest = LARC_Q;
    } else {
      lt->dest = LARC_G;
      if (queue_length(&larc->larc_g) + 1 > lt->g_size) {
        lt->g_evict = true;
      }
    }
  }
}

bool larc_transaction_is_legal(struct larc_policy *larc,
                               struct larc_transaction *lt) {
  bool legal = true;

  if (lt->c_evict) {
    legal &= queue_length(&larc->larc_q) > 0;
  }

  return legal;
}

void larc_remove_entry(struct larc_policy *larc, struct larc_entry *le) {
  struct cache_nucleus *cn = &larc->nucleus;
  struct entry *e = &le->e;
  if (!e->migrating) {
    queue_remove(&le->larc_list);
    if (in_cache(cn, e)) {
      inc_demotions(&cn->stats);
    }
  }
  cache_nucleus_remove(cn, e);
}

void larc_evict(struct larc_policy *larc, struct queue *q,
                struct cache_nucleus_result *result) {
  struct cache_nucleus *cn = &larc->nucleus;
  struct larc_entry *le = queue_peek_entry(q, struct larc_entry, larc_list);
  struct entry *demoted = &le->e;
  if (result != NULL) {
    result->old_oblock = demoted->oblock;
    result->dirty_eviction = demoted->dirty;
  }
  larc_remove_entry(larc, le);
}

void larc_transaction_commit(struct larc_policy *larc,
                             struct larc_transaction *lt,
                             struct cache_nucleus_result *result) {
  struct cache_nucleus *cn = &larc->nucleus;

  larc->g_size = lt->g_size;

  result->op = CACHE_NUCLEUS_NEW;

  if (lt->c_evict) {
    larc_evict(larc, &larc->larc_q, result);
    result->op = CACHE_NUCLEUS_REPLACE;
  }

  if (lt->g_hit) {
    struct entry *e = hash_lookup(&cn->ht, lt->oblock);
    struct larc_entry *le = to_larc_entry(e);
    larc_remove_entry(larc, le);
  }

  if (lt->dest == LARC_Q) {
    struct entry *e;
    struct larc_entry *le;

    if (lt->c_hit) {
      e = hash_lookup(&cn->ht, lt->oblock);
      le = to_larc_entry(e);

      result->cblock = infer_cblock(&cn->cache_pool, e);

      if (!e->migrating) {
        queue_remove(&le->larc_list);
      }

      inc_hits(&cn->stats);
      result->op = CACHE_NUCLEUS_HIT;
    } else {
      e = insert_in_cache(cn, lt->oblock, result);
      le = to_larc_entry(e);
      migrating_entry(&cn->cache_pool, e);

      inc_misses(&cn->stats);
    }

    if (!e->migrating) {
      queue_push(&larc->larc_q, &le->larc_list);
    }
  } else {
    struct entry *e = insert_in_meta(cn, lt->oblock);
    struct larc_entry *le = to_larc_entry(e);
    queue_push(&larc->larc_g, &le->larc_list);

    inc_filters(&cn->stats);
    result->op = CACHE_NUCLEUS_FILTER;
  }

  if (lt->g_evict) {
    larc_evict(larc, &larc->larc_g, NULL);
  }
}

void larc_remove_api(struct cache_nucleus *cn, oblock_t oblock,
                     bool remove_to_history) {
  struct larc_policy *larc = to_larc_policy(cn);
  struct entry *e = hash_lookup(&cn->ht, oblock);
  struct larc_entry *le = to_larc_entry(e);

  larc_remove_entry(larc, le);
}

void larc_insert_api(struct cache_nucleus *cn, oblock_t oblock,
                     cblock_t cblock) {
  struct larc_policy *larc = to_larc_policy(cn);
  struct entry *e = insert_in_cache_at(cn, oblock, cblock, NULL);
  struct larc_entry *le = to_larc_entry(e);

  queue_push(&larc->larc_q, &le->larc_list);
}

void larc_migrated_api(struct cache_nucleus *cn, oblock_t oblock) {
  struct larc_policy *larc = to_larc_policy(cn);
  struct entry *e = hash_lookup(&cn->ht, oblock);
  struct larc_entry *le;

  LOG_ASSERT(e != NULL);
  LOG_ASSERT(e->migrating);

  le = to_larc_entry(e);
  migrated_entry(&cn->cache_pool, e);
  queue_push(&larc->larc_q, &le->larc_list);

  inc_promotions(&cn->stats);
}

int larc_map_api(struct cache_nucleus *cn, oblock_t oblock, bool write,
                 struct cache_nucleus_result *result) {
  struct entry *e = hash_lookup(&cn->ht, oblock);
  struct larc_policy *larc = to_larc_policy(cn);
  struct larc_transaction lt;

  larc_transaction_init(larc, &lt, oblock);
  larc_transaction_set(larc, &lt);
  if (larc_transaction_is_legal(larc, &lt)) {
    larc_transaction_commit(larc, &lt, result);

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

void larc_destroy_api(struct cache_nucleus *cn) {
  struct larc_policy *larc = to_larc_policy(cn);
  epool_exit(&cn->cache_pool);
  epool_exit(&cn->meta_pool);
  policy_exit(cn);
  mem_free(larc);
}

enum cache_nucleus_operation
larc_next_victim_api(struct cache_nucleus *cn, oblock_t oblock,
                     struct entry **next_victim_p) {
  struct larc_policy *larc = to_larc_policy(cn);
  struct larc_transaction lt;

  *next_victim_p = NULL;

  larc_transaction_init(larc, &lt, oblock);
  larc_transaction_set(larc, &lt);

  if (larc_transaction_is_legal(larc, &lt)) {
    if (lt.c_evict) {
      struct larc_entry *le =
          queue_peek_entry(&larc->larc_q, struct larc_entry, larc_list);
      *next_victim_p = &le->e;
      return CACHE_NUCLEUS_REPLACE;
    }
    if (lt.g_hit) {
      return CACHE_NUCLEUS_NEW;
    }
    return CACHE_NUCLEUS_FILTER;
  }
  return CACHE_NUCLEUS_FAIL;
}

int larc_init(struct larc_policy *larc, cblock_t cache_size,
              cblock_t meta_size) {
  struct cache_nucleus *cn = &larc->nucleus;

  cache_nucleus_init_api(
      cn, larc_map_api, larc_remove_api, larc_insert_api, larc_migrated_api,
      default_cache_lookup, default_meta_lookup, default_cblock_lookup,
      default_remap, larc_destroy_api, default_get_stats, default_set_time,
      default_residency, default_cache_is_full, default_infer_cblock,
      larc_next_victim_api);

  queue_init(&larc->larc_q);
  queue_init(&larc->larc_g);

  larc->g_size = meta_size / 10;

  epool_create(&cn->cache_pool, cache_size, struct larc_entry, e);
  if (!cn->cache_pool.entries_begin || !cn->cache_pool.entries) {
    LOG_DEBUG("epool_create for cache_pool failed");
    goto cache_epool_create_fail;
  }
  epool_create(&cn->meta_pool, meta_size, struct larc_entry, e);
  if (!cn->meta_pool.entries_begin || !cn->meta_pool.entries) {
    LOG_DEBUG("epool_create for meta_pool failed");
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

struct cache_nucleus *larc_create(cblock_t cache_size, cblock_t meta_size) {
  int r;
  struct cache_nucleus *cn;
  struct larc_policy *larc = mem_alloc(sizeof(*larc));
  if (larc == NULL) {
    LOG_DEBUG("mem_alloc failed");
    goto larc_create_fail;
  }

  cn = &larc->nucleus;

  r = larc_init(larc, cache_size, meta_size);
  if (r) {
    // TODO: error
    goto larc_init_fail;
  }

  return cn;

larc_init_fail:
  larc_destroy_api(cn);
larc_create_fail:
  return NULL;
}
