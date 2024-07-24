#include "mstar_policy.h"
#include "cache_nucleus.h"
#include "cache_nucleus_internal.h"
#include "common.h"
#include "entry.h"
#include "mstar_logic.h"
#include "mstar_policy_struct.h"
#include "tools/queue.h"

void mstar_only_remove(struct mstar_policy *mstar, struct mstar_entry *me) {
  struct entry *e;
  LOG_ASSERT(me != NULL);
  e = &me->e;
  queue_remove(&me->mstar_list);
  // removes entry from hashtable and epool
  cache_nucleus_remove(&mstar->nucleus, &me->e);
}

// TODO passed entry isn't from m*, but rather internal_policy?
//      ^^^ CORRECT! mstar is mostly invisible to the outside!
//      TODO TODO make this into a note later
void mstar_remove(struct cache_nucleus *cn, oblock_t oblock,
                  bool remove_to_history) {
  struct mstar_policy *mstar = to_mstar_policy(cn);
  struct cache_nucleus *internal_policy = mstar->internal_policy;
  //  NOTE: get different entry* from mstar's hashtable instead
  struct entry *meta_e = hash_lookup(&cn->ht, oblock);
  if (meta_e != NULL && !remove_to_history) {
    mstar_only_remove(mstar, to_mstar_entry(meta_e));
  }
  policy_remove(internal_policy, oblock, remove_to_history);
}

void mstar_insert(struct cache_nucleus *cn, oblock_t oblock, cblock_t cblock) {
  struct mstar_policy *mstar = to_mstar_policy(cn);
  struct cache_nucleus *internal_policy = mstar->internal_policy;
  policy_insert(internal_policy, oblock, cblock);
}

void mstar_migrated(struct cache_nucleus *cn, oblock_t oblock) {
  struct mstar_policy *mstar = to_mstar_policy(cn);
  struct cache_nucleus *internal_policy = mstar->internal_policy;
  policy_migrated(internal_policy, oblock);
}

// eviction is m*'s meta_pool only
void mstar_evict(struct mstar_policy *mstar) {
  struct mstar_entry *me =
      queue_pop_entry(&mstar->mstar_g, struct mstar_entry, mstar_list);
  LOG_ASSERT(me != NULL);
  mstar_only_remove(mstar, me);
}

void mstar_ghost_length_control(struct mstar_policy *mstar) {
  if (queue_length(&mstar->mstar_g) > mstar->ghost_size) {
    mstar_evict(mstar);
  }
}

void mstar_increase_ghost_size(struct mstar_policy *mstar) {
  struct cache_nucleus *cn = &mstar->nucleus;
  unsigned cache_size = cn->meta_size;
  unsigned ghost_size = mstar->ghost_size;

  LOG_ASSERT(ghost_size > 0);
  mstar->ghost_size =
      min((9 * cache_size) / 10, ghost_size + (cache_size / ghost_size));
}

void mstar_decrease_ghost_size(struct mstar_policy *mstar) {
  struct cache_nucleus *cn = &mstar->nucleus;
  unsigned cache_size = cn->meta_size;
  unsigned ghost_size = mstar->ghost_size;
  unsigned tmp;

  LOG_ASSERT(cache_size > ghost_size);
  // NOTE: avoiding negatives when tmp > ghost_size
  tmp = cache_size / (cache_size - ghost_size);
  if (ghost_size >= tmp) {
    mstar->ghost_size = max(cache_size / 10, ghost_size - tmp);
  } else {
    mstar->ghost_size = cache_size / 10;
  }
}

int mstar_ghost_miss(struct mstar_policy *mstar, oblock_t oblock, bool write,
                     struct cache_nucleus_result *result) {
  int r = 0;
  struct cache_nucleus *internal_policy = mstar->internal_policy;

  // TODO checkup on
  if (policy_cache_is_full(internal_policy)) {
    struct entry *e = insert_in_meta(&mstar->nucleus, oblock);
    struct mstar_entry *me = to_mstar_entry(e);

    queue_push(&mstar->mstar_g, &me->mstar_list);

    mstar_increase_ghost_size(mstar);

    if (mstar->logic.state->id != UNSTABLE) {
      goto not_caching;
    }
  }

  r = policy_map(internal_policy, oblock, write, result);
  return r;

not_caching:
  inc_filters(&mstar->nucleus.stats);
  inc_ios(&mstar->nucleus.stats); // keep total ios accurate for get_stats()
  return r;
}

int mstar_ghost_hit(struct mstar_policy *mstar, oblock_t oblock, bool write,
                    struct cache_nucleus_result *result) {
  int r;
  struct cache_nucleus *internal_policy = mstar->internal_policy;
  struct cache_nucleus *cn = &mstar->nucleus;

  if (policy_cache_is_full(internal_policy)) {
    struct entry *e = hash_lookup(&cn->ht, oblock);
    // TODO checkup on
    mstar_only_remove(mstar, to_mstar_entry(e));

    if (mstar->logic.state->id == UNSTABLE) {
      // TODO checkup on
      struct mstar_entry *me;

      e = insert_in_meta(cn, oblock);
      me = to_mstar_entry(e);

      queue_push(&mstar->mstar_g, &me->mstar_list);
    }

    mstar_increase_ghost_size(mstar);
  }

  r = policy_map(internal_policy, oblock, write, result);

  return r;
}

int mstar_hit(struct mstar_policy *mstar, oblock_t oblock, bool write,
              struct cache_nucleus_result *result) {
  int r;
  struct cache_nucleus *internal_policy = mstar->internal_policy;
  struct mstar_logic *logic = &mstar->logic;

  r = policy_map(internal_policy, oblock, write, result);
  if (mstar->logic.state->id != UNSTABLE ||
      policy_cache_is_full(internal_policy)) {
    mstar_decrease_ghost_size(mstar);
  }

  return r;
}

int mstar_map(struct cache_nucleus *cn, oblock_t oblock, bool write,
              struct cache_nucleus_result *result) {
  int r = 0;
  cblock_t cblock;
  struct mstar_policy *mstar = to_mstar_policy(cn);
  struct cache_nucleus *internal_policy = mstar->internal_policy;
  struct entry *internal_entry = policy_cache_lookup(internal_policy, oblock);
  struct entry *ghost_entry = hash_lookup(&cn->ht, oblock);

  bool cache_hit = internal_entry != NULL;
  bool ghost_hit = ghost_entry != NULL;

  result->op = CACHE_NUCLEUS_FILTER;

  if (cache_hit) {
    r = mstar_hit(mstar, oblock, write, result);
  } else if (ghost_hit) {
    r = mstar_ghost_hit(mstar, oblock, write, result);
  } else {
    r = mstar_ghost_miss(mstar, oblock, write, result);
  }

  mstar_ghost_length_control(mstar);

  // NOTE: originally before hit/miss logic
  update_mstar(&mstar->logic, cache_hit, ghost_hit);

  return r;
}

struct entry *mstar_cache_lookup(struct cache_nucleus *cn, oblock_t oblock) {
  struct mstar_policy *mstar = to_mstar_policy(cn);
  struct cache_nucleus *internal_policy = mstar->internal_policy;
  return policy_cache_lookup(internal_policy, oblock);
}

struct entry *mstar_meta_lookup(struct cache_nucleus *cn, oblock_t oblock) {
  struct mstar_policy *mstar = to_mstar_policy(cn);
  struct cache_nucleus *internal_policy = mstar->internal_policy;
  return policy_meta_lookup(internal_policy, oblock);
}

struct entry *mstar_cblock_lookup(struct cache_nucleus *cn, cblock_t cblock) {
  struct mstar_policy *mstar = to_mstar_policy(cn);
  struct cache_nucleus *internal_policy = mstar->internal_policy;
  return policy_cblock_lookup(internal_policy, cblock);
}

void mstar_remap(struct cache_nucleus *cn, oblock_t current_oblock,
                 oblock_t new_oblock) {
  struct mstar_policy *mstar = to_mstar_policy(cn);
  struct cache_nucleus *internal_policy = mstar->internal_policy;
  struct hashtable *ht = &mstar->nucleus.ht;

  struct entry *e = hash_lookup(ht, current_oblock);
  hash_remove(e);
  e->oblock = new_oblock;
  hash_insert(ht, e);

  policy_remap(internal_policy, current_oblock, new_oblock);
}

void mstar_destroy(struct cache_nucleus *cn) {
  struct mstar_policy *mstar = to_mstar_policy(cn);
  struct cache_nucleus *internal_p = mstar->internal_policy;
  if (internal_p != NULL) {
    policy_destroy(internal_p);
  }
  epool_exit(&mstar->nucleus.meta_pool);
  mstar_logic_exit(&mstar->logic);
  policy_exit(&mstar->nucleus);
  mem_free(mstar);
}

void mstar_get_stats(struct cache_nucleus *cn, struct policy_stats *stats) {
  struct mstar_policy *mstar = to_mstar_policy(cn);
  struct cache_nucleus *internal_policy = mstar->internal_policy;
  policy_get_stats(internal_policy, stats);
  stats->filters += cn->stats.filters;
}

void mstar_set_time(struct cache_nucleus *cn, unsigned time) {
  struct mstar_policy *mstar = to_mstar_policy(cn);
  struct cache_nucleus *internal_policy = mstar->internal_policy;
  policy_set_time(internal_policy, time);
  cn->time = time;
}

cblock_t mstar_residency(struct cache_nucleus *cn) {
  struct mstar_policy *mstar = to_mstar_policy(cn);
  struct cache_nucleus *internal_policy = mstar->internal_policy;
  return policy_residency(internal_policy);
}

bool mstar_cache_is_full(struct cache_nucleus *cn) {
  struct mstar_policy *mstar = to_mstar_policy(cn);
  struct cache_nucleus *internal_policy = mstar->internal_policy;
  return policy_cache_is_full(internal_policy);
}

cblock_t mstar_infer_cblock(struct cache_nucleus *cn, struct entry *e) {
  struct mstar_policy *mstar = to_mstar_policy(cn);
  struct cache_nucleus *internal_policy = mstar->internal_policy;
  return policy_infer_cblock(internal_policy, e);
}

enum cache_nucleus_operation mstar_next_victim(struct cache_nucleus *cn,
                                               oblock_t oblock,
                                               struct entry **next_victim_p) {
  struct mstar_policy *mstar = to_mstar_policy(cn);
  struct entry *meta_e = hash_lookup(&cn->ht, oblock);
  bool meta_hit = meta_e != NULL;

  struct cache_nucleus *internal_policy = mstar->internal_policy;
  struct entry *internal_e = policy_cache_lookup(internal_policy, oblock);
  bool internal_hit = internal_e != NULL;

  *next_victim_p = NULL;

  if (internal_hit || meta_hit || mstar->logic.state->id == UNSTABLE) {
    return policy_next_victim(internal_policy, oblock, next_victim_p);
  }

  return CACHE_NUCLEUS_FILTER;
}

int mstar_init(struct mstar_policy *mstar, cblock_t cache_size,
               cblock_t meta_size, struct cache_nucleus *internal_policy,
               bool adaptive) {
  struct cache_nucleus *cn = &mstar->nucleus;
  int r;

  cache_nucleus_init_api(
      cn, mstar_map, mstar_remove, mstar_insert, mstar_migrated,
      mstar_cache_lookup, mstar_meta_lookup, mstar_cblock_lookup, mstar_remap,
      mstar_destroy, mstar_get_stats, mstar_set_time, mstar_residency,
      mstar_cache_is_full, mstar_infer_cblock, mstar_next_victim);

  mstar->internal_policy = internal_policy;

  // TODO mstar meta lru queue
  queue_init(&mstar->mstar_g);

  // NOTE: set ghost_size to max (aka 90% cache size)
  mstar->ghost_size = (9u * cache_size) / 10u;

  epool_create(&cn->meta_pool, cache_size, struct mstar_entry, e);
  if (!cn->meta_pool.entries_begin || !cn->meta_pool.entries) {
    LOG_DEBUG("epool_create for meta_pool failed");
    goto meta_epool_create_fail;
  }

  if (mstar_logic_init(&mstar->logic, adaptive, cache_size)) {
    LOG_DEBUG("init_logic failed");
    goto init_logic_fail;
  }

  if (policy_init(&mstar->nucleus, 0, cache_size,
                  CACHE_NUCLEUS_SINGLE_THREAD)) {
    LOG_DEBUG("policy_init failed");
    goto policy_init_fail;
  }

  return 0;

policy_init_fail:
  mstar_logic_exit(&mstar->logic);
init_logic_fail:
  epool_exit(&cn->meta_pool);
meta_epool_create_fail:
  return -ENOSPC;
}

struct cache_nucleus *
mstar_create(cblock_t cache_size, cblock_t meta_size,
             struct cache_nucleus *(*policy_create)(cblock_t cache_size,
                                                    cblock_t meta_size),
             bool adaptive) {
  int r;
  struct cache_nucleus *cn;
  struct cache_nucleus *policy_cn;
  struct mstar_policy *mstar = mem_alloc(sizeof(*mstar));

  if (!mstar) {
    LOG_DEBUG("mem_alloc failed. insufficient memory");
    goto mstar_create_fail;
  }

  cn = &mstar->nucleus;

  policy_cn = policy_create(cache_size, meta_size);
  if (!policy_cn) {
    LOG_DEBUG("policy_create failed");
    goto policy_create_fail;
  }

  r = mstar_init(mstar, cache_size, meta_size, policy_cn, adaptive);
  if (r) {
    LOG_DEBUG("mstar_init failed. error %d", r);
    goto mstar_init_fail;
  }

  return cn;

mstar_init_fail:
policy_create_fail:
  mstar_destroy(cn);
mstar_create_fail:
  return NULL;
}
