#include "alecar_policy.h"
#include "alecar_learning_rate.h"
#include "cache_nucleus_internal.h"
#include "common.h"
#include "entry.h"
#include "tools/fixedptc.h"
#include "tools/heap.h"
#include "tools/queue.h"
#include "tools/random.h"

#define NUM_ALECAR_LISTS 2
enum alecar_list { ALECAR_CACHE, ALECAR_HIST };
enum alecar_evict_blame { ALECAR_LRU, ALECAR_LFU };

struct alecar_policy {
  struct cache_nucleus nucleus;
  struct queue alecar_q[NUM_ALECAR_LISTS];
  struct heap lfu;

  struct alecar_learning_rate learning_rate;
  fixedpt W[2];

  struct random_state random_state;

  // TODO new
  unsigned history_size; // multiple???
};

struct alecar_entry {
  struct entry e;
  struct queue_head alecar_list;
  enum alecar_list list_id;
  unsigned time_at_evict;
  struct heap_head lfu_list;
  enum alecar_evict_blame evicted_me;
};

struct alecar_policy *to_alecar_policy(struct cache_nucleus *cn) {
  return container_of(cn, struct alecar_policy, nucleus);
}

struct alecar_entry *to_alecar_entry(struct entry *e) {
  return container_of(e, struct alecar_entry, e);
}

int alecar_compare(struct heap_head *a, struct heap_head *b) {
  if (a->key == b->key) {
    struct alecar_entry *le_a = container_of(a, struct alecar_entry, lfu_list);
    struct alecar_entry *le_b = container_of(b, struct alecar_entry, lfu_list);
    unsigned time_a = le_a->e.time;
    unsigned time_b = le_b->e.time;

    if (time_a < time_b) {
      return -1;
    }
    if (time_a > time_b) {
      return 1;
    }
    return 0;
  }
  if (a->key < b->key) {
    return -1;
  }
  return 1;
}

// TODO add notes for this
void alecar_adjust_weights(struct alecar_policy *alecar,
                           struct alecar_entry *le) {
  enum alecar_evict_blame blame = le->evicted_me;
  fixedpt reward[2] = {0, 0};

  if (blame == ALECAR_LRU) {
    // TODO figure out a new name for reward
    reward[0] = -FIXEDPT_ONE;
    //    -fixedpt_pow(alecar->error_discount_rate, fixedpt_fromint(delay));
  } else {
    reward[1] = -FIXEDPT_ONE;
    //    -fixedpt_pow(alecar->error_discount_rate, fixedpt_fromint(delay));
  }

  {
    fixedpt upper_limit = fixedpt_rconst(0.99);
    fixedpt lower_limit = fixedpt_rconst(0.01);

    fixedpt exp[2] = {fixedpt_exp(fixedpt_mul(
                          alecar->learning_rate.learning_rate, reward[0])),
                      fixedpt_exp(fixedpt_mul(
                          alecar->learning_rate.learning_rate, reward[1]))};
    fixedpt Wexp[2] = {fixedpt_mul(alecar->W[0], exp[0]),
                       fixedpt_mul(alecar->W[1], exp[1])};
    fixedpt Wsum = fixedpt_add(Wexp[0], Wexp[1]);

    alecar->W[0] = fixedpt_div(Wexp[0], Wsum);
    alecar->W[1] = fixedpt_div(Wexp[1], Wsum);

    if (alecar->W[0] >= upper_limit) {
      alecar->W[0] = upper_limit;
      alecar->W[1] = lower_limit;
    } else if (alecar->W[1] >= upper_limit) {
      alecar->W[0] = lower_limit;
      alecar->W[1] = upper_limit;
    }
  }
}

void alecar_remove_entry(struct cache_nucleus *cn, struct entry *e) {
  struct alecar_policy *alecar = to_alecar_policy(cn);
  struct alecar_entry *le = to_alecar_entry(e);

  queue_remove(&le->alecar_list);
  if (in_cache(cn, e)) {
    heap_delete(&alecar->lfu, &le->lfu_list);
  }
  cache_nucleus_remove(cn, e);
}

struct entry *alecar_insert_in_cache(struct alecar_policy *alecar,
                                     oblock_t oblock, unsigned freq,
                                     struct cache_nucleus_result *result) {
  struct cache_nucleus *cn = &alecar->nucleus;
  struct entry *e = insert_in_cache(cn, oblock, result);
  struct alecar_entry *le = to_alecar_entry(e);
  queue_push(&alecar->alecar_q[ALECAR_CACHE], &le->alecar_list);
  le->list_id = ALECAR_CACHE;
  e->time = cn->time;

  // this was key = 1. that would be a serious bug and reason for the difference
  heap_insert(&alecar->lfu, &le->lfu_list, freq);

  return e;
}

void alecar_meta_evict(struct alecar_policy *alecar, struct queue *q) {
  struct cache_nucleus *cn = &alecar->nucleus;
  struct alecar_entry *le =
      queue_pop_entry(q, struct alecar_entry, alecar_list);
  struct entry *e = &le->e;
  alecar_remove_entry(cn, e);
}

struct entry *alecar_insert_in_meta(struct alecar_policy *alecar,
                                    oblock_t oblock,
                                    enum alecar_evict_blame eviction_policy,
                                    unsigned freq) {
  struct cache_nucleus *cn = &alecar->nucleus;
  struct queue *evict_queue = &alecar->alecar_q[ALECAR_HIST];
  struct alecar_entry *le;
  struct entry *e;

  if (queue_length(evict_queue) == alecar->history_size) {
    alecar_meta_evict(alecar, evict_queue);
  }

  e = insert_in_meta(cn, oblock);
  le = to_alecar_entry(e);

  // not put in LFU heap as only cached entries need to go there
  queue_push(&alecar->alecar_q[ALECAR_HIST], &le->alecar_list);
  le->list_id = ALECAR_HIST;
  le->lfu_list.key = freq;
  le->evicted_me = eviction_policy;

  return e;
}

fixedpt alecar_random(struct alecar_policy *alecar) {
  unsigned limit = 100000;
  unsigned random = random_int(&alecar->random_state) % limit;
  return fixedpt_div(fixedpt_fromint(random), fixedpt_fromint(limit));
}

bool alecar_cache_evict(struct alecar_policy *alecar,
                        oblock_t *evicted_oblock) {
  struct cache_nucleus *cn = &alecar->nucleus;
  struct alecar_entry *lru_entry = queue_peek_entry(
      &alecar->alecar_q[ALECAR_CACHE], struct alecar_entry, alecar_list);
  struct alecar_entry *lfu_entry =
      heap_min_entry(&alecar->lfu, struct alecar_entry, lfu_list);
  struct alecar_entry *demoted = lru_entry;
  enum alecar_evict_blame eviction_policy;
  bool dirty_evict = false;

  inc_demotions(&cn->stats);

  LOG_ASSERT(lru_entry != NULL);
  LOG_ASSERT(lfu_entry != NULL);

  if (lru_entry == lfu_entry) {
    inc_private_stat(&cn->stats);
  }

  if (alecar_random(alecar) < alecar->W[0]) {
    demoted = lru_entry;
    eviction_policy = ALECAR_LRU;
  } else {
    demoted = lfu_entry;
    eviction_policy = ALECAR_LFU;
  }

  *evicted_oblock = demoted->e.oblock;
  dirty_evict = demoted->e.dirty;

  {
    unsigned saved_freq = demoted->lfu_list.key;
    struct entry *e;
    struct alecar_entry *le;

    alecar_remove_entry(cn, &demoted->e);
    e = alecar_insert_in_meta(alecar, *evicted_oblock, eviction_policy,
                              saved_freq);
    le = to_alecar_entry(e);
    le->time_at_evict = cn->time;
  }

  return dirty_evict;
}

void alecar_meta_miss(struct alecar_policy *alecar, oblock_t oblock,
                      struct cache_nucleus_result *result) {
  struct cache_nucleus *cn = &alecar->nucleus;

  if (cache_is_full(cn)) {
    result->dirty_eviction = alecar_cache_evict(alecar, &result->old_oblock);
  }

  alecar_insert_in_cache(alecar, oblock, 1, result);
}

void alecar_meta_hit(struct alecar_policy *alecar, oblock_t oblock,
                     struct cache_nucleus_result *result) {
  struct cache_nucleus *cn = &alecar->nucleus;
  struct entry *e = hash_lookup(&cn->ht, oblock);
  struct alecar_entry *le = to_alecar_entry(e);
  unsigned old_freq = le->lfu_list.key;

  alecar_adjust_weights(alecar, le);

  alecar_remove_entry(cn, e);

  if (cache_is_full(cn)) {
    alecar_cache_evict(alecar, &result->old_oblock);
  }

  alecar_insert_in_cache(alecar, oblock, old_freq + 1, result);
}

void alecar_miss(struct alecar_policy *alecar, oblock_t oblock,
                 struct cache_nucleus_result *result) {
  struct cache_nucleus *cn = &alecar->nucleus;
  struct entry *e = hash_lookup(&cn->ht, oblock);

  inc_misses(&cn->stats);
  inc_promotions(&cn->stats);

  if (cache_is_full(cn)) {
    result->op = CACHE_NUCLEUS_REPLACE;
  } else {
    result->op = CACHE_NUCLEUS_NEW;
  }

  if (in_meta(cn, e)) {
    alecar_meta_hit(alecar, oblock, result);
  } else {
    alecar_meta_miss(alecar, oblock, result);
  }
}

void alecar_hit(struct alecar_policy *alecar, struct entry *e,
                struct cache_nucleus_result *result) {
  struct cache_nucleus *cn = &alecar->nucleus;
  struct alecar_entry *le = to_alecar_entry(e);

  inc_hits(&cn->stats);

  result->op = CACHE_NUCLEUS_HIT;
  result->cblock = infer_cblock(e);

  queue_remove(&le->alecar_list);
  queue_push(&alecar->alecar_q[ALECAR_CACHE], &le->alecar_list);
  le->list_id = ALECAR_CACHE;

  ++(le->lfu_list.key);
  heapify(&alecar->lfu, le->lfu_list.index);

  e->time = cn->time;

  update_cache_hit(&alecar->learning_rate);
}

void alecar_clean_up(struct alecar_policy *alecar) {
  // purpose is to reset when 2N entries are in the priority queue (heap)
  // since i'm not placing things in the heap for simplicity's sake this is
  // difficult..
  // TODO
  // i could reverse my decision and we can reverse the reverse again later
  struct cache_nucleus *cn = &alecar->nucleus;
  // TODO cache_size + meta_size or 2 * cache_size?
  if (alecar->lfu.size > 2 * cn->cache_size) {
    struct heap *lfu = &alecar->lfu;
    cblock_t i;

    // go through heap's array. remove meta as we go
    // should be safe, since lfu->size changes as we go
    for (i = 0; i < lfu->size; i++) {
      struct heap_head *hh = lfu->heap_arr[i];
      struct alecar_entry *le = container_of(hh, struct alecar_entry, lfu_list);

      if (in_meta(cn, &le->e)) {
        // swap to last
        // delete lfu_list entry
        heap_delete(lfu, hh);
        // hh->key = 0; it's not removing from freq in python, only heap
        // due to heapify in heap_delete, we gotta do this index again
        i--;
      }
    }
  }
}

void alecar_remove(struct cache_nucleus *cn, oblock_t oblock) {
  struct entry *e = hash_lookup(&cn->ht, oblock);
  if (in_cache(cn, e)) {
    inc_demotions(&cn->stats);
  }
  alecar_remove_entry(cn, e);
}

void alecar_remove_preserve(struct cache_nucleus *cn, oblock_t oblock) {
  struct entry *e = hash_lookup(&cn->ht, oblock);
  // frequency and list_id stays in entry
  alecar_remove_entry(cn, e);
}

void alecar_insert(struct cache_nucleus *cn, struct entry *e) {
  struct alecar_policy *alecar = to_alecar_policy(cn);
  struct alecar_entry *le = to_alecar_entry(e);
  unsigned freq = 1;

  if (e->preserved) {
    freq = le->lfu_list.key;
    e->preserved = false;
  }

  e = insert_in_cache_at(cn, e->oblock, infer_cblock(e), NULL);
  le = to_alecar_entry(e);
  e->time = cn->time;

  queue_push(&alecar->alecar_q[ALECAR_CACHE], &le->alecar_list);
  le->list_id = ALECAR_CACHE;

  heap_insert(&alecar->lfu, &le->lfu_list, freq);
}

int alecar_map(struct cache_nucleus *cn, oblock_t oblock, bool write,
               struct cache_nucleus_result *result) {
  struct entry *e = hash_lookup(&cn->ht, oblock);
  struct alecar_policy *alecar = to_alecar_policy(cn);

  inc_ios(&cn->stats);

  result->op = CACHE_NUCLEUS_MISS;

  // protect from trying to evict when it can't
  alecar_clean_up(alecar);

  if (cn->time % alecar->learning_rate.sequence_length == 0) {
    update_learning_rate(&alecar->learning_rate);
  }

  if (in_cache(cn, e)) {
    alecar_hit(alecar, e, result);
  } else {
    alecar_miss(alecar, oblock, result);
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

void alecar_destroy(struct cache_nucleus *cn) {
  struct alecar_policy *alecar = to_alecar_policy(cn);

  heap_exit(&alecar->lfu);

  epool_exit(&cn->cache_pool);
  epool_exit(&cn->meta_pool);
  policy_exit(cn);
  mem_free(alecar);
}

int alecar_preload_cache_entry(struct cache_nucleus *cn, oblock_t oblock,
                               cblock_t cblock) {
  struct alecar_policy *alecar = to_alecar_policy(cn);
  struct entry *e = insert_in_cache_at(cn, oblock, cblock, NULL);
  struct alecar_entry *le;
  if (e == NULL) {
    return -ENOENT;
  }

  le = to_alecar_entry(e);
  queue_push(&alecar->alecar_q[ALECAR_CACHE], &le->alecar_list);
  le->list_id = ALECAR_CACHE;
  heap_insert(&alecar->lfu, &le->lfu_list, 1);

  return 0;
}

int alecar_init(struct alecar_policy *alecar, cblock_t cache_size,
                cblock_t meta_size) {
  int i;
  struct cache_nucleus *cn = &alecar->nucleus;

  cache_nucleus_init_api(cn, alecar_map, alecar_remove, alecar_remove_preserve,
                         alecar_insert, default_lookup, default_cblock_lookup,
                         default_remap, alecar_destroy, default_get_stats,
                         default_set_time, default_residency,
                         default_cache_is_full);

  queue_init(&alecar->alecar_q[ALECAR_CACHE]);
  queue_init(&alecar->alecar_q[ALECAR_HIST]);

  // NOTE: can be changed. i'm just putting in a placeholder seed
  prandom_init_seed(&alecar->random_state, 42);

  alecar->W[0] = FIXEDPT_ONE_HALF;
  alecar->W[1] = FIXEDPT_ONE_HALF;

  alecar->history_size = meta_size;

  alecar_learning_rate_init(&alecar->learning_rate, cache_size);

  epool_create(&cn->cache_pool, cache_size, struct alecar_entry, e);
  if (!cn->cache_pool.entries_begin || !cn->cache_pool.entries) {
    LOG_DEBUG("epool_create for cache_pool failed.");
    goto cache_epool_create_fail;
  }
  // TODO reduce this?
  epool_create(&cn->meta_pool, alecar->history_size, struct alecar_entry, e);
  if (!cn->meta_pool.entries_begin || !cn->meta_pool.entries) {
    LOG_DEBUG("epool_create for meta_pool failed.");
    goto meta_epool_create_fail;
  }

  // TODO reduce this?
  if (heap_init(&alecar->lfu, cache_size + meta_size)) {
    LOG_DEBUG("heap_init failed.");
    goto heap_init_fail;
  }

  heap_set_compare(&alecar->lfu, alecar_compare);

  if (policy_init(cn, cache_size, meta_size)) {
    LOG_DEBUG("policy_init failed");
    goto policy_init_fail;
  }

  return 0;

policy_init_fail:
  heap_exit(&alecar->lfu);
heap_init_fail:
  epool_exit(&cn->meta_pool);
meta_epool_create_fail:
  epool_exit(&cn->cache_pool);
cache_epool_create_fail:
  return -ENOSPC;
}

struct cache_nucleus *alecar_create(cblock_t cache_size, cblock_t meta_size) {
  int r;
  struct cache_nucleus *cn;
  struct alecar_policy *alecar = mem_alloc(sizeof(*alecar));
  if (alecar == NULL) {
    LOG_DEBUG("mem_alloc failed");
    goto alecar_create_fail;
  }

  cn = &alecar->nucleus;

  r = alecar_init(alecar, cache_size, meta_size);
  if (r) {
    LOG_DEBUG("alecar init failed. error %d", r);
    goto alecar_init_fail;
  }

  return cn;

alecar_init_fail:
  alecar_destroy(cn);
alecar_create_fail:
  return NULL;
}
