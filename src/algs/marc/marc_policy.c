#include "../arc/arc_policy.h"
#include "cache_nucleus_internal.h"
#include "marc_policy_struct.h"

struct marc_transaction {
  oblock_t oblock;
  bool write;
  uint64_t tabu_counter;
  enum marc_state state;
  uint64_t ghost_size;
  bool c_hit;
  bool save_sample_avg;
  bool sample_clear;
  bool sample_avg_clear;
  bool phase_clear;
  bool save_ghost_hits;
  bool ghost_remove;
  bool ghost_add;
  bool filter;
};

void marc_transaction_init(struct marc_policy *marc,
                           struct marc_transaction *mt, oblock_t oblock,
                           bool write) {
  mt->oblock = oblock;
  mt->write = write;
  mt->tabu_counter = marc->tabu_counter;
  mt->state = marc->state;
  mt->ghost_size = marc->ghost_size;
  mt->c_hit = false;
  mt->save_sample_avg = false;
  mt->sample_clear = false;
  mt->sample_avg_clear = false;
  mt->phase_clear = false;
  mt->save_ghost_hits = false;
  mt->ghost_remove = false;
  mt->ghost_add = false;
  mt->filter = true;
}

void marc_change_state(struct marc_policy *marc, struct marc_transaction *mt,
                       enum marc_state state) {
  struct cache_nucleus *cn = &marc->nucleus;
  struct cache_nucleus *arc = marc->internal;
  if (state != UNSTABLE) {
    mt->ghost_size = arc->cache_size / 10;
  } else {
    mt->tabu_counter = 2 * marc->TABU;
  }

  mt->state = state;
  mt->sample_clear = true;
  mt->sample_avg_clear = true;
  mt->phase_clear = state != STABLE;
}

uint64_t marc_ghost_decrease_size(struct marc_policy *marc,
                                  uint64_t ghost_size) {
  struct cache_nucleus *arc = marc->internal;
  uint64_t cache_size = arc->cache_size;
  uint64_t tmp;

  if (ghost_size == cache_size) {
    return cache_size / 10;
  }

  tmp = cache_size / (cache_size - ghost_size);
  if (ghost_size >= tmp) {
    return max(cache_size / 10, ghost_size - tmp);
  } else {
    return cache_size / 10;
  }
}

uint64_t marc_ghost_increase_size(struct marc_policy *marc,
                                  uint64_t ghost_size) {
  struct cache_nucleus *arc = marc->internal;
  uint64_t cache_size = arc->cache_size;
  return min((9 * cache_size) / 10, ghost_size + (cache_size / ghost_size));
}

bool marc_unst_uniq_check(struct marc_policy *marc, uint64_t hr_sample,
                          uint64_t hr_state) {
  return (hr_state / 2 > hr_sample) || (hr_sample < 5);
}

bool marc_unst_stab_check(struct marc_policy *marc, uint64_t hr_sample,
                          uint64_t hr_state) {
  return (hr_state < (11 * hr_sample) / 10 &&
          hr_state > (9 * hr_sample) / 10) ||
         (hr_sample >= hr_state && hr_sample >= 20);
}

bool marc_stab_unst_check(struct marc_policy *marc, uint64_t hr_sample,
                          uint64_t hr_state) {
  return hr_sample <= (7 * hr_state) / 10;
}

bool marc_uniq_unst_check(struct marc_policy *marc, uint64_t hr_sample,
                          uint64_t hr_state) {
  uint64_t filter_hr =
      ((marc->ghost_hits - marc->ghost_hits_old) * 100) / marc->TABU;
  return (filter_hr > 10) || (hr_sample > 5);
}

void marc_try_transition(struct marc_policy *marc, struct marc_transaction *mt,
                         uint64_t hr_sample, uint64_t hr_state) {
  if (marc->state == UNSTABLE) {
    if (marc_unst_uniq_check(marc, hr_sample, hr_state)) {
      marc_change_state(marc, mt, UNIQUE);
    } else if (marc_unst_stab_check(marc, hr_sample, hr_state)) {
      marc_change_state(marc, mt, STABLE);
    }
  } else if (marc->state == STABLE) {
    if (marc_stab_unst_check(marc, hr_sample, hr_state)) {
      marc_change_state(marc, mt, UNSTABLE);
    }
  } else if (marc->state == UNIQUE) {
    if (marc_uniq_unst_check(marc, hr_sample, hr_state)) {
      marc_change_state(marc, mt, UNSTABLE);
    }
  }
}

void marc_transaction_set(struct marc_policy *marc,
                          struct marc_transaction *mt) {
  struct cache_nucleus *cn = &marc->nucleus;
  struct cache_nucleus *arc = marc->internal;
  struct entry *e = policy_cache_lookup(arc, mt->oblock);

  if (marc->tabu_counter > 0) {
    mt->tabu_counter--;
  }

  mt->c_hit = e != NULL;

  bool sample_full = marc->sample.len + 1 >= marc->sample.size;
  if (mt->tabu_counter == 0 && sample_full) {
    mt->tabu_counter = marc->TABU;

    uint64_t hr_sample =
        (100 *
         (marc->sample.sum - window_stats_peek(&marc->sample) + mt->c_hit)) /
        marc->sample.size;
    uint64_t hr_state =
        (100 * (marc->phase_hits + mt->c_hit)) / (marc->phase_ios + 1);

    mt->save_sample_avg = true;

    marc_try_transition(marc, mt, hr_sample, hr_state);
    mt->save_ghost_hits = true;
  }

  if (mt->state != UNSTABLE) {
    if (mt->c_hit) {
      mt->filter = false;
      mt->ghost_size = marc_ghost_decrease_size(marc, mt->ghost_size);
    } else {
      if (policy_cache_is_full(arc)) {
        bool g_hit = hash_lookup(&cn->ht, mt->oblock) != NULL;
        if (g_hit) {
          mt->ghost_remove = true;
        }
        // NOTE: old code seemed to be impossible to do meta lookup equivalent
        // if (g_hit || policy_meta_lookup(arc, mt->oblock) != NULL) {
        if (g_hit) {
          mt->filter = false;
        } else {
          mt->ghost_add = true;
        }
        mt->ghost_size = marc_ghost_increase_size(marc, mt->ghost_size);
      } else {
        mt->filter = false;
      }
    }
  } else {
    if (policy_cache_is_full(arc)) {
      if (mt->c_hit) {
        mt->ghost_size = marc_ghost_decrease_size(marc, mt->ghost_size);
      } else {
        if (hash_lookup(&cn->ht, mt->oblock) != NULL) {
          mt->ghost_remove = true;
        }
        mt->ghost_add = true;
        mt->ghost_size = marc_ghost_increase_size(marc, mt->ghost_size);
      }
    }
    mt->filter = false;
  }
}

bool marc_transaction_is_legal(struct marc_policy *marc,
                               struct marc_transaction *mt) {
  bool legal = true;

  if (!mt->filter) {
    struct cache_nucleus *arc = marc->internal;
    struct entry *e;
    legal &= policy_next_victim(arc, mt->oblock, &e) != CACHE_NUCLEUS_FAIL;
  }

  return legal;
}

void marc_remove_entry(struct marc_policy *marc, struct marc_entry *me) {
  queue_remove(&me->marc_list);
  cache_nucleus_remove(&marc->nucleus, &me->e);
}

void marc_ghost_evict(struct marc_policy *marc) {
  struct marc_entry *me =
      queue_peek_entry(&marc->ghost_q, struct marc_entry, marc_list);
  marc_remove_entry(marc, me);
}

void marc_transaction_commit(struct marc_policy *marc,
                             struct marc_transaction *mt,
                             struct cache_nucleus_result *result) {
  struct cache_nucleus *cn = &marc->nucleus;
  struct cache_nucleus *arc = marc->internal;

  marc->tabu_counter = mt->tabu_counter;
  marc->ghost_size = mt->ghost_size;
  marc->state = mt->state;

  window_stats_push(&marc->sample, mt->c_hit);
  marc->phase_hits += mt->c_hit;
  marc->phase_ios++;

  if (mt->save_sample_avg) {
    window_stats_push(&marc->sample_avg,
                      (100 * marc->sample.sum) / marc->sample.size);
  }

  if (mt->sample_clear) {
    window_stats_clear(&marc->sample);
  }
  if (mt->sample_avg_clear) {
    window_stats_clear(&marc->sample);
  }
  if (mt->phase_clear) {
    marc->phase_hits = 0;
    marc->phase_ios = 0;
  }

  if (mt->save_ghost_hits) {
    // TODO any alternative style
    marc->ghost_hits_old = marc->ghost_hits;
  }

  if (mt->ghost_remove && mt->ghost_add) {
    // ghost hit
    struct entry *e = hash_lookup(&cn->ht, mt->oblock);
    struct marc_entry *me = to_marc_entry(e);
    marc->ghost_hits++;
    queue_remove(&me->marc_list);
    queue_push(&marc->ghost_q, &me->marc_list);
  } else if (mt->ghost_remove) {
    struct entry *e = hash_lookup(&cn->ht, mt->oblock);
    struct marc_entry *me = to_marc_entry(e);
    marc_remove_entry(marc, me);
  } else if (mt->ghost_add) {
    struct entry *e = insert_in_meta(cn, mt->oblock);
    struct marc_entry *me = to_marc_entry(e);
    queue_push(&marc->ghost_q, &me->marc_list);
  }

  if (!mt->filter) {
    policy_map(arc, mt->oblock, mt->write, result);
  } else {
    inc_filters(&cn->stats);
    result->op = CACHE_NUCLEUS_FILTER;
  }

  if (queue_length(&marc->ghost_q) > marc->ghost_size) {
    marc_ghost_evict(marc);
  }
}

void marc_remove_api(struct cache_nucleus *cn, oblock_t oblock,
                     bool remove_to_history) {
  struct marc_policy *marc = to_marc_policy(cn);
  struct cache_nucleus *arc = marc->internal;
  policy_remove(arc, oblock, remove_to_history);
}

void marc_insert_api(struct cache_nucleus *cn, oblock_t oblock,
                     cblock_t cblock) {
  struct marc_policy *marc = to_marc_policy(cn);
  struct cache_nucleus *arc = marc->internal;
  policy_insert(arc, oblock, cblock);
}

void marc_migrated_api(struct cache_nucleus *cn, oblock_t oblock) {
  struct marc_policy *marc = to_marc_policy(cn);
  struct cache_nucleus *arc = marc->internal;
  policy_migrated(arc, oblock);
}

int marc_map_api(struct cache_nucleus *cn, oblock_t oblock, bool write,
                 struct cache_nucleus_result *result) {
  struct marc_policy *marc = to_marc_policy(cn);
  struct marc_transaction mt;

  marc_transaction_init(marc, &mt, oblock, write);
  marc_transaction_set(marc, &mt);
  if (marc_transaction_is_legal(marc, &mt)) {
    marc_transaction_commit(marc, &mt, result);

    // TODO write stuff
  } else {
    result->op = CACHE_NUCLEUS_FAIL;
  }

  return 0;
}

struct entry *marc_cache_lookup_api(struct cache_nucleus *cn, oblock_t oblock) {
  struct marc_policy *marc = to_marc_policy(cn);
  struct cache_nucleus *arc = marc->internal;
  return policy_cache_lookup(arc, oblock);
}

struct entry *marc_meta_lookup_api(struct cache_nucleus *cn, oblock_t oblock) {
  struct marc_policy *marc = to_marc_policy(cn);
  struct cache_nucleus *arc = marc->internal;
  return policy_meta_lookup(arc, oblock);
}

struct entry *marc_cblock_lookup_api(struct cache_nucleus *cn,
                                     cblock_t cblock) {
  struct marc_policy *marc = to_marc_policy(cn);
  struct cache_nucleus *arc = marc->internal;
  return policy_cblock_lookup(arc, cblock);
}

void marc_remap_api(struct cache_nucleus *cn, oblock_t current_oblock,
                    oblock_t new_oblock) {
  struct marc_policy *marc = to_marc_policy(cn);
  struct cache_nucleus *arc = marc->internal;
  policy_remap(arc, current_oblock, new_oblock);
}

void marc_destroy_api(struct cache_nucleus *cn) {
  struct marc_policy *marc = to_marc_policy(cn);
  struct cache_nucleus *arc = marc->internal;

  policy_destroy(arc);

  window_stats_exit(&marc->sample_avg);
  window_stats_exit(&marc->sample);
  epool_exit(&cn->meta_pool);
  policy_exit(cn);
  mem_free(marc);
}

void marc_get_stats_api(struct cache_nucleus *cn, struct policy_stats *stats) {
  struct marc_policy *marc = to_marc_policy(cn);
  struct cache_nucleus *arc = marc->internal;
  policy_get_stats(arc, stats);
  stats->filters += cn->stats.filters;
}

void marc_set_time_api(struct cache_nucleus *cn, unsigned time) {
  struct marc_policy *marc = to_marc_policy(cn);
  struct cache_nucleus *arc = marc->internal;
  policy_set_time(arc, time);
  cn->time = time;
}

unsigned marc_residency_api(struct cache_nucleus *cn) {
  struct marc_policy *marc = to_marc_policy(cn);
  struct cache_nucleus *arc = marc->internal;
  return policy_residency(arc);
}

bool marc_cache_is_full_api(struct cache_nucleus *cn) {
  struct marc_policy *marc = to_marc_policy(cn);
  struct cache_nucleus *arc = marc->internal;
  return policy_cache_is_full(arc);
}

cblock_t marc_infer_cblock_api(struct cache_nucleus *cn, struct entry *e) {
  struct marc_policy *marc = to_marc_policy(cn);
  struct cache_nucleus *arc = marc->internal;
  return policy_infer_cblock(arc, e);
}

enum cache_nucleus_operation
marc_next_victim_api(struct cache_nucleus *cn, oblock_t oblock,
                     struct entry **next_victim_p) {
  struct marc_policy *marc = to_marc_policy(cn);
  struct cache_nucleus *arc = marc->internal;
  struct marc_transaction mt;

  *next_victim_p = NULL;

  marc_transaction_init(marc, &mt, oblock, false);
  marc_transaction_set(marc, &mt);
  if (marc_transaction_is_legal(marc, &mt)) {
    if (!mt.filter) {
      return policy_next_victim(arc, oblock, next_victim_p);
    }
    return CACHE_NUCLEUS_FILTER;
  }
  return CACHE_NUCLEUS_FAIL;
}

int marc_init(struct marc_policy *marc, cblock_t cache_size, cblock_t meta_size,
              struct cache_nucleus *arc) {
  struct cache_nucleus *cn = &marc->nucleus;

  cache_nucleus_init_api(
      cn, marc_map_api, marc_remove_api, marc_insert_api, marc_migrated_api,
      marc_cache_lookup_api, marc_meta_lookup_api, marc_cblock_lookup_api,
      marc_remap_api, marc_destroy_api, marc_get_stats_api, marc_set_time_api,
      marc_residency_api, marc_cache_is_full_api, marc_infer_cblock_api,
      marc_next_victim_api);

  epool_create(&cn->meta_pool, cache_size, struct marc_entry, e);
  if (!cn->meta_pool.entries_begin || !cn->meta_pool.entries) {
    LOG_DEBUG("epool_create for meta_pool failed.");
    goto meta_epool_create_fail;
  }

  if (policy_init(cn, 0, meta_size, CACHE_NUCLEUS_SINGLE_THREAD)) {
    LOG_DEBUG("policy_init failed");
    goto policy_init_fail;
  }

  marc->internal = arc;

  marc->TABU = cache_size;
  marc->tabu_counter = 2 * marc->TABU;

  marc->state = UNSTABLE;

  queue_init(&marc->ghost_q);
  marc->ghost_size = cache_size;
  marc->ghost_hits = 0;
  marc->ghost_hits_old = 0;

  window_stats_init(&marc->sample, cache_size);
  if (!marc->sample.values) {
    goto sample_window_init_fail;
  }

  window_stats_init(&marc->sample_avg, 1);
  if (!marc->sample_avg.values) {
    goto sample_avg_window_init_fail;
  }

  marc->phase_hits = 0;
  marc->phase_ios = 0;

  return 0;

sample_avg_window_init_fail:
  window_stats_exit(&marc->sample);
sample_window_init_fail:
  policy_exit(cn);
policy_init_fail:
  epool_exit(&cn->meta_pool);
meta_epool_create_fail:
  return -ENOSPC;
}

struct cache_nucleus *marc_create(cblock_t cache_size, cblock_t meta_size) {
  int r;
  struct cache_nucleus *cn;
  struct cache_nucleus *arc;

  struct marc_policy *marc = mem_alloc(sizeof(*marc));
  if (!marc) {
    LOG_DEBUG("mem_alloc failed. insufficient memory");
    goto marc_create_fail;
  }

  cn = &marc->nucleus;

  arc = arc_create(cache_size, meta_size);
  if (!arc) {
    LOG_DEBUG("arc_create failed");
    goto arc_create_fail;
  }

  r = marc_init(marc, cache_size, meta_size, arc);
  if (r) {
    LOG_DEBUG("marc_initfailed. error %d", r);
    goto marc_init_fail;
  }

  return cn;

marc_init_fail:
  policy_destroy(arc);
arc_create_fail:
  marc_destroy_api(cn);
marc_create_fail:
  return NULL;
}
