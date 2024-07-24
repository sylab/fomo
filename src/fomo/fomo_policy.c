#include "fomo_policy.h"
#include "cache_nucleus.h"
#include "cache_nucleus_internal.h"
#include "common.h"
#include "entry.h"
#include "fomo_policy_struct.h"
#include "tools/queue.h"

struct fomo_transaction {
  oblock_t oblock;
  bool filter;
  enum fomo_state state;
  bool c_hit;
  bool mh_hit;
  uint64_t c_hits;
  uint64_t c_hits_curr;
  uint64_t mh_hits;
  uint64_t mh_hits_curr;
  unsigned mh_hits_total;
  unsigned counter;
};

void fomo_remove_entry(struct fomo_policy *fomo, struct fomo_entry *fe) {
  queue_remove(&fe->fomo_list);
  cache_nucleus_remove(&fomo->nucleus, &fe->e);
}

void fomo_remove(struct cache_nucleus *cn, oblock_t oblock,
                 bool remove_to_history) {
  struct fomo_policy *fomo = to_fomo_policy(cn);
  struct cache_nucleus *internal_policy = fomo->internal_policy;
  struct entry *meta_e = hash_lookup(&cn->ht, oblock);

  if (meta_e != NULL && !remove_to_history) {
    struct fomo_entry *fe = to_fomo_entry(meta_e);
    fomo_remove_entry(fomo, fe);
  }
  policy_remove(internal_policy, oblock, remove_to_history);
}

void fomo_transaction_init(struct fomo_policy *fomo,
                           struct fomo_transaction *ft, oblock_t oblock) {
  ft->oblock = oblock;
  ft->filter = true;
  ft->state = fomo->state;
  ft->c_hit = false;
  ft->mh_hit = false;
  ft->c_hits = fomo->c_hits;
  ft->mh_hits = fomo->mh_hits;
  ft->mh_hits_total = fomo->mh_hits_total;
  ft->counter = fomo->counter;
}

void fomo_transaction_set(struct fomo_policy *fomo,
                          struct fomo_transaction *ft) {
  struct cache_nucleus *cn = &fomo->nucleus;
  struct cache_nucleus *internal_policy = fomo->internal_policy;
  struct entry *fomo_e = hash_lookup(&cn->ht, ft->oblock);
  struct entry *internal_e = policy_cache_lookup(internal_policy, ft->oblock);

  ft->c_hit = internal_e != NULL;
  ft->mh_hit = fomo_e != NULL;

  if (ft->c_hit) {
    ft->c_hits += 1000;
    ft->filter = false;
  }

  if (ft->mh_hit) {
    ft->mh_hits += 1000;
    ft->filter = false;
    ++ft->mh_hits_total;
  }

  ++ft->counter;

  if ((ft->counter) % fomo->period_size == 0) {
    if (cache_is_full(internal_policy)) {
      // determine state
      // ===============
      // minimum threshold for HR_misshistory of 5% in order to switch states
      // NOTE: hr > (5/100) * period_size
      //       => thousandths
      //       hr thous > (5/100) * (1000 thous/1) * period_size
      //       hr thous > 50 * period_size thous
      if (ft->state == FOMO_FILTER &&
          (ft->mh_hits > (50 * fomo->period_size)) &&
          ft->c_hits < ft->mh_hits) {
        ft->state = FOMO_INSERT;
      } else if (ft->state == FOMO_INSERT &&
                 //(ft->c_hits_curr >= (3 * ft->mh_hits_curr) / 2) &&
                 (ft->c_hits >= ft->mh_hits)) {
        ft->state = FOMO_FILTER;
      }
    }

    // apply decay
    ft->c_hits = 0;
    ft->mh_hits = 0;
    ft->counter = 0;
  }

  if (ft->state == FOMO_INSERT) {
    ft->filter = false;
  } else {
    ft->filter &= true;
  }
}

bool fomo_transaction_is_legal(struct fomo_policy *fomo,
                               struct fomo_transaction *ft) {
  struct cache_nucleus *internal_policy = fomo->internal_policy;
  bool legal = true;

  if (!ft->filter) {
    struct entry *next_victim;
    legal &= policy_next_victim(internal_policy, ft->oblock, &next_victim) !=
             CACHE_NUCLEUS_FAIL;
  }
  return legal;
}

void fomo_transaction_commit(struct fomo_policy *fomo,
                             struct fomo_transaction *ft,
                             struct cache_nucleus_result *result) {
  struct cache_nucleus *cn = &fomo->nucleus;
  struct cache_nucleus *internal_policy = fomo->internal_policy;

  fomo->counter = ft->counter;
  fomo->c_hits = ft->c_hits;
  fomo->mh_hits = ft->mh_hits;
  fomo->state = ft->state;
  fomo->mh_hits_total = ft->mh_hits_total;

  if (ft->c_hit) {
    if (ft->mh_hit) {
      struct entry *e = hash_lookup(&cn->ht, ft->oblock);
      struct fomo_entry *fe = to_fomo_entry(e);
      fomo_remove_entry(fomo, fe);
    }
  } else if (ft->mh_hit) {
    struct entry *e = hash_lookup(&cn->ht, ft->oblock);
    struct fomo_entry *fe = to_fomo_entry(e);

    if (fomo->state == FOMO_FILTER) {
      fomo_remove_entry(fomo, fe);
    } else {
      // move fomo entry to mru position
      queue_remove(&fe->fomo_list);
      queue_push(&fomo->fomo_mh, &fe->fomo_list);
    }
  } else {
    struct entry *e;
    struct fomo_entry *fe;

    // evict from miss history if needed
    if (queue_length(&fomo->fomo_mh) == fomo->ghost_size) {
      struct fomo_entry *lru_fe =
          queue_peek_entry(&fomo->fomo_mh, struct fomo_entry, fomo_list);
      fomo_remove_entry(fomo, lru_fe);
    }

    // add fomo entry at mru position
    e = insert_in_meta(cn, ft->oblock);
    fe = to_fomo_entry(e);
    queue_push(&fomo->fomo_mh, &fe->fomo_list);
  }

  // either filter or allow the access to go to the cache
  if (ft->filter) {
    inc_filters(&cn->stats);
    result->op = CACHE_NUCLEUS_FILTER;
  } else {
    // TODO pass write info to internal policy for dirty eviction info?
    policy_map(internal_policy, ft->oblock, false, result);
  }
}

// API functions
int fomo_map(struct cache_nucleus *cn, oblock_t oblock, bool write,
             struct cache_nucleus_result *result) {
  struct fomo_policy *fomo = to_fomo_policy(cn);
  struct fomo_transaction ft;

  result->op = CACHE_NUCLEUS_FAIL;

  fomo_transaction_init(fomo, &ft, oblock);
  fomo_transaction_set(fomo, &ft);
  if (fomo_transaction_is_legal(fomo, &ft)) {
    fomo_transaction_commit(fomo, &ft, result);
  } else {
    result->op = CACHE_NUCLEUS_FAIL;
  }

  return 0;
}

void fomo_insert(struct cache_nucleus *cn, oblock_t oblock, cblock_t cblock) {
  struct cache_nucleus *internal_policy = get_internal_policy(cn);
  policy_insert(internal_policy, oblock, cblock);
}

void fomo_migrated(struct cache_nucleus *cn, oblock_t oblock) {
  struct cache_nucleus *internal_policy = get_internal_policy(cn);
  policy_migrated(internal_policy, oblock);
}

struct entry *fomo_cache_lookup(struct cache_nucleus *cn, oblock_t oblock) {
  struct cache_nucleus *internal_policy = get_internal_policy(cn);
  return policy_cache_lookup(internal_policy, oblock);
}

struct entry *fomo_meta_lookup(struct cache_nucleus *cn, oblock_t oblock) {
  struct cache_nucleus *internal_policy = get_internal_policy(cn);
  return policy_meta_lookup(internal_policy, oblock);
}

struct entry *fomo_cblock_lookup(struct cache_nucleus *cn, cblock_t cblock) {
  struct cache_nucleus *internal_policy = get_internal_policy(cn);
  return policy_cblock_lookup(internal_policy, cblock);
}

void fomo_remap(struct cache_nucleus *cn, oblock_t current_oblock,
                oblock_t new_oblock) {
  struct fomo_policy *fomo = to_fomo_policy(cn);
  struct cache_nucleus *internal_policy = fomo->internal_policy;

  struct entry *e = hash_lookup(&cn->ht, new_oblock);
  if (e != NULL) {
    struct fomo_entry *fe = to_fomo_entry(e);
    fomo_remove_entry(fomo, fe);
  }

  e = hash_lookup(&cn->ht, current_oblock);
  if (e != NULL) {
    hash_remove(e);
    e->oblock = new_oblock;
    hash_insert(&cn->ht, e);
  }

  policy_remap(internal_policy, current_oblock, new_oblock);
}

void fomo_destroy(struct cache_nucleus *cn) {
  struct fomo_policy *fomo = to_fomo_policy(cn);
  struct cache_nucleus *internal_policy = fomo->internal_policy;
  policy_destroy(internal_policy);
  epool_exit(&cn->meta_pool);
  policy_exit(cn);
  mem_free(fomo);
}

void fomo_get_stats(struct cache_nucleus *cn, struct policy_stats *stats) {
  struct cache_nucleus *internal_policy = get_internal_policy(cn);
  policy_get_stats(internal_policy, stats);
  stats->filters += cn->stats.filters;
}

void fomo_set_time(struct cache_nucleus *cn, unsigned time) {
  struct cache_nucleus *internal_policy = get_internal_policy(cn);
  cn->time = time;
  policy_set_time(internal_policy, time);
}

unsigned fomo_residency(struct cache_nucleus *cn) {
  struct cache_nucleus *internal_policy = get_internal_policy(cn);
  return policy_residency(internal_policy);
}

bool fomo_cache_is_full(struct cache_nucleus *cn) {
  struct cache_nucleus *internal_policy = get_internal_policy(cn);
  return policy_cache_is_full(internal_policy);
}

cblock_t fomo_infer_cblock(struct cache_nucleus *cn, struct entry *e) {
  struct cache_nucleus *internal_policy = get_internal_policy(cn);
  return policy_infer_cblock(internal_policy, e);
}

enum cache_nucleus_operation fomo_next_victim(struct cache_nucleus *cn,
                                              oblock_t oblock,
                                              struct entry **next_victim) {
  struct fomo_policy *fomo = to_fomo_policy(cn);
  struct cache_nucleus *internal_policy = fomo->internal_policy;
  struct fomo_transaction ft;

  *next_victim = NULL;

  fomo_transaction_init(fomo, &ft, oblock);
  fomo_transaction_set(fomo, &ft);
  if (fomo_transaction_is_legal(fomo, &ft)) {

    if (ft.filter) {
      return CACHE_NUCLEUS_FILTER;
    } else {
      return policy_next_victim(internal_policy, oblock, next_victim);
    }
  }
  return CACHE_NUCLEUS_FAIL;
}

// init and create
int fomo_init(struct fomo_policy *fomo, cblock_t cache_size, cblock_t meta_size,
              struct cache_nucleus *internal_policy) {
  struct cache_nucleus *cn = &fomo->nucleus;
  unsigned i;

  cache_nucleus_init_api(
      cn, fomo_map, fomo_remove, fomo_insert, fomo_migrated, fomo_cache_lookup,
      fomo_meta_lookup, fomo_cblock_lookup, fomo_remap, fomo_destroy,
      fomo_get_stats, fomo_set_time, fomo_residency, fomo_cache_is_full,
      fomo_infer_cblock, fomo_next_victim);

  fomo->internal_policy = internal_policy;

  fomo->state = FOMO_INSERT;

  queue_init(&fomo->fomo_mh);

  fomo->ghost_size = cache_size;
  fomo->period_size = max(cache_size / 100, 1u);
  fomo->counter = 0;

  fomo->c_hits = 0;
  fomo->mh_hits = 0;
  fomo->mh_hits_total = 0;

  epool_create(&cn->meta_pool, cache_size, struct fomo_entry, e);
  if (!cn->meta_pool.entries_begin || !cn->meta_pool.entries) {
    LOG_DEBUG("epool_create for meta_pool failed");
    goto meta_epool_create_fail;
  }

  if (policy_init(&fomo->nucleus, 0, cache_size, CACHE_NUCLEUS_SINGLE_THREAD)) {
    LOG_DEBUG("policy_init failed");
    goto policy_init_fail;
  }

  return 0;

policy_init_fail:
  epool_exit(&cn->meta_pool);
meta_epool_create_fail:
  return -ENOSPC;
}

struct cache_nucleus *fomo_create(cblock_t cache_size, cblock_t meta_size,
                                  policy_create_f policy_create) {
  int r;
  struct cache_nucleus *cn;
  struct cache_nucleus *policy_cn;
  struct fomo_policy *fomo = mem_alloc(sizeof(*fomo));

  if (!fomo) {
    LOG_DEBUG("mem_alloc failed. insufficient memory");
    goto fomo_create_fail;
  }

  cn = &fomo->nucleus;

  policy_cn = policy_create(cache_size, meta_size);
  if (!policy_cn) {
    LOG_DEBUG("policy_create failed");
    goto policy_create_fail;
  }

  r = fomo_init(fomo, cache_size, meta_size, policy_cn);
  if (r) {
    LOG_DEBUG("fomo_init failed. error %d", r);
    goto fomo_init_fail;
  }

  return cn;

fomo_init_fail:
policy_create_fail:
  fomo_destroy(cn);
fomo_create_fail:
  return NULL;
}
