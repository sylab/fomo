#include "lecar_policy.h"
#include "cache_nucleus_internal.h"
#include "common.h"
#include "entry.h"
#include "tools/fixedptc.h"
#include "tools/heap.h"
#include "tools/queue.h"
#include "tools/random.h"

#define NUM_LECAR_LISTS 2
enum lecar_list { LECAR_CACHE, LECAR_HIST };
enum lecar_evict_blame { LECAR_LRU, LECAR_LFU };

struct lecar_policy {
  struct cache_nucleus nucleus;
  struct queue lecar_q[NUM_LECAR_LISTS];
  struct heap lfu;

  fixedpt error_discount_rate;
  fixedpt learning_rate;

  fixedpt W[2];

  struct random_state random_state;

  // TODO new
  unsigned history_size; // multiple???
};

struct lecar_entry {
  struct entry e;
  struct queue_head lecar_list;
  enum lecar_list list_id;
  unsigned time_at_evict;
  struct heap_head lfu_list;
  enum lecar_evict_blame evicted_me;
};

struct lecar_policy *to_lecar_policy(struct cache_nucleus *cn) {
  return container_of(cn, struct lecar_policy, nucleus);
}

struct lecar_entry *to_lecar_entry(struct entry *e) {
  return container_of(e, struct lecar_entry, e);
}

int lecar_compare(struct heap_head *a, struct heap_head *b) {
  if (a->key == b->key) {
    struct lecar_entry *le_a = container_of(a, struct lecar_entry, lfu_list);
    struct lecar_entry *le_b = container_of(b, struct lecar_entry, lfu_list);
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
void lecar_adjust_weights(struct lecar_policy *lecar, struct lecar_entry *le) {
  struct cache_nucleus *cn = &lecar->nucleus;
  enum lecar_evict_blame blame = le->evicted_me;
  fixedpt reward[2] = {0, 0};
  unsigned delay = cn->time - le->time_at_evict;
  // just to be safe
  reward[0] = reward[1] = fixedpt_fromint(0);

  if (blame == LECAR_LRU) {
    // TODO figure out a new name for reward
    reward[0] =
        -fixedpt_pow(lecar->error_discount_rate, fixedpt_fromint(delay));
  } else {
    reward[1] =
        -fixedpt_pow(lecar->error_discount_rate, fixedpt_fromint(delay));
  }

  {
    fixedpt exp[2] = {
        fixedpt_exp(fixedpt_mul(lecar->learning_rate, reward[0])),
        fixedpt_exp(fixedpt_mul(lecar->learning_rate, reward[1]))};
    fixedpt Wexp[2] = {fixedpt_mul(lecar->W[0], exp[0]),
                       fixedpt_mul(lecar->W[1], exp[1])};
    fixedpt Wsum = fixedpt_add(Wexp[0], Wexp[1]);
    lecar->W[0] = fixedpt_div(Wexp[0], Wsum);
    lecar->W[1] = fixedpt_div(Wexp[1], Wsum);
  }
}

void lecar_remove_entry(struct cache_nucleus *cn, struct entry *e) {
  struct lecar_policy *lecar = to_lecar_policy(cn);
  struct lecar_entry *le = to_lecar_entry(e);
  queue_remove(&le->lecar_list);
  if (in_cache(cn, e)) {
    heap_delete(&lecar->lfu, &le->lfu_list);
  }
  cache_nucleus_remove(cn, e);
}

struct entry *lecar_insert_in_cache(struct lecar_policy *lecar, oblock_t oblock,
                                    unsigned freq,
                                    struct cache_nucleus_result *result) {
  struct cache_nucleus *cn = &lecar->nucleus;
  struct entry *e = insert_in_cache(cn, oblock, result);
  struct lecar_entry *le = to_lecar_entry(e);
  queue_push(&lecar->lecar_q[LECAR_CACHE], &le->lecar_list);
  le->list_id = LECAR_CACHE;

  e->time = cn->time;

  // this was key = 1. that would be a serious bug and reason for the difference
  heap_insert(&lecar->lfu, &le->lfu_list, freq);

  return e;
}

void lecar_meta_evict(struct lecar_policy *lecar, struct queue *q) {
  struct cache_nucleus *cn = &lecar->nucleus;
  struct lecar_entry *le = queue_pop_entry(q, struct lecar_entry, lecar_list);
  struct entry *e = &le->e;
  lecar_remove_entry(cn, e);
}

struct entry *lecar_insert_in_meta(struct lecar_policy *lecar, oblock_t oblock,
                                   enum lecar_evict_blame eviction_policy,
                                   unsigned freq) {
  struct cache_nucleus *cn = &lecar->nucleus;
  struct queue *evict_queue = &lecar->lecar_q[LECAR_HIST];
  struct lecar_entry *le;
  struct entry *e;

  if (queue_length(evict_queue) == lecar->history_size) {
    lecar_meta_evict(lecar, evict_queue);
  }

  e = insert_in_meta(cn, oblock);
  le = to_lecar_entry(e);

  // not put in LFU heap as only IS_VALID cache entries allowed there
  queue_push(&lecar->lecar_q[LECAR_HIST], &le->lecar_list);
  le->list_id = LECAR_HIST;
  le->lfu_list.key = freq;
  le->evicted_me = eviction_policy;

  return e;
}

fixedpt lecar_random(struct lecar_policy *lecar) {
  unsigned limit = 100000;
  unsigned random = random_int(&lecar->random_state) % limit;
  return fixedpt_div(fixedpt_fromint(random), fixedpt_fromint(limit));
}

bool lecar_cache_evict(struct lecar_policy *lecar, oblock_t *evicted_oblock) {
  struct cache_nucleus *cn = &lecar->nucleus;
  struct lecar_entry *lru_entry = queue_peek_entry(
      &lecar->lecar_q[LECAR_CACHE], struct lecar_entry, lecar_list);
  struct lecar_entry *lfu_entry =
      heap_min_entry(&lecar->lfu, struct lecar_entry, lfu_list);
  struct lecar_entry *demoted;
  enum lecar_evict_blame eviction_policy;
  bool dirty;

  LOG_ASSERT(lru_entry != NULL);
  LOG_ASSERT(lfu_entry != NULL);

  demoted = lru_entry;

  if (lru_entry == lfu_entry) {
    inc_private_stat(&cn->stats);
  }

  if (lecar_random(lecar) < lecar->W[0]) {
    demoted = lru_entry;
    eviction_policy = LECAR_LRU;
  } else {
    demoted = lfu_entry;
    eviction_policy = LECAR_LFU;
  }

  *evicted_oblock = demoted->e.oblock;
  dirty = demoted->e.dirty;

  {
    unsigned saved_freq = demoted->lfu_list.key;
    struct entry *e;
    struct lecar_entry *le;

    lecar_remove_entry(cn, &demoted->e);
    inc_demotions(&cn->stats);

    e = lecar_insert_in_meta(lecar, *evicted_oblock, eviction_policy,
                             saved_freq);
    le = to_lecar_entry(e);
    le->time_at_evict = cn->time;
  }

  return dirty;
}

void lecar_meta_miss(struct lecar_policy *lecar, oblock_t oblock,
                     struct cache_nucleus_result *result) {
  struct cache_nucleus *cn = &lecar->nucleus;

  if (cache_is_full(cn)) {
    result->dirty_eviction = lecar_cache_evict(lecar, &result->old_oblock);
  }

  lecar_insert_in_cache(lecar, oblock, 1, result);
}

void lecar_meta_hit(struct lecar_policy *lecar, oblock_t oblock,
                    struct cache_nucleus_result *result) {
  struct cache_nucleus *cn = &lecar->nucleus;
  struct entry *e = hash_lookup(&cn->ht, oblock);
  struct lecar_entry *le = to_lecar_entry(e);
  unsigned old_freq = le->lfu_list.key;

  lecar_adjust_weights(lecar, le);

  lecar_remove_entry(cn, e);

  if (cache_is_full(cn)) {
    lecar_cache_evict(lecar, &result->old_oblock);
  }

  lecar_insert_in_cache(lecar, oblock, old_freq + 1, result);
}

void lecar_miss(struct lecar_policy *lecar, oblock_t oblock,
                struct cache_nucleus_result *result) {
  struct cache_nucleus *cn = &lecar->nucleus;
  struct entry *e = hash_lookup(&cn->ht, oblock);

  inc_misses(&cn->stats);
  inc_promotions(&cn->stats);

  if (cache_is_full(cn)) {
    result->op = CACHE_NUCLEUS_REPLACE;
  } else {
    result->op = CACHE_NUCLEUS_NEW;
  }

  if (in_meta(cn, e)) {
    lecar_meta_hit(lecar, oblock, result);
  } else {
    lecar_meta_miss(lecar, oblock, result);
  }
}

void lecar_hit(struct lecar_policy *lecar, struct entry *e,
               struct cache_nucleus_result *result) {
  struct cache_nucleus *cn = &lecar->nucleus;
  struct lecar_entry *le = to_lecar_entry(e);

  inc_hits(&cn->stats);

  result->op = CACHE_NUCLEUS_HIT;
  result->cblock = infer_cblock(e);

  queue_remove(&le->lecar_list);
  queue_push(&lecar->lecar_q[LECAR_CACHE], &le->lecar_list);
  le->list_id = LECAR_CACHE;

  ++(le->lfu_list.key);
  heapify(&lecar->lfu, le->lfu_list.index);

  e->time = cn->time;
}

void lecar_clean_up(struct lecar_policy *lecar) {
  // purpose is to reset when 2N entries are in the priority queue (heap)
  // since i'm not placing things in the heap for simplicity's sake this is
  // difficult..
  // TODO
  // i could reverse my decision and we can reverse the reverse again later
  struct cache_nucleus *cn = &lecar->nucleus;
  // TODO should it just be cache_size + meta_size instead of 2 * cache_size?
  if (lecar->lfu.size > 2 * cn->cache_size) {
    struct heap *lfu = &lecar->lfu;
    cblock_t i;

    // go through heap's array. remove meta as we go
    // should be safe, since lfu->size changes as we go
    for (i = 0; i < lfu->size; i++) {
      struct heap_head *hh = lfu->heap_arr[i];
      struct lecar_entry *le = container_of(hh, struct lecar_entry, lfu_list);

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

void lecar_remove(struct cache_nucleus *cn, oblock_t oblock) {
  struct entry *e = hash_lookup(&cn->ht, oblock);
  if (in_cache(cn, e)) {
    inc_demotions(&cn->stats);
  }
  lecar_remove_entry(cn, e);
}

// It is assumed that oblock is in cache
void lecar_remove_preserve(struct cache_nucleus *cn, oblock_t oblock) {
  struct entry *e = hash_lookup(&cn->ht, oblock);

  // NOTE: lecar_remove_entry does not erase frequency information
  lecar_remove_entry(cn, e);

  e->preserved = true;
}

void lecar_insert(struct cache_nucleus *cn, struct entry *e) {
  struct lecar_policy *lecar = to_lecar_policy(cn);
  struct lecar_entry *le = to_lecar_entry(e);
  unsigned freq = 1;

  if (e->preserved) {
    freq = le->lfu_list.key;
    e->preserved = false;
  }

  e = insert_in_cache_at(cn, e->oblock, infer_cblock(e), NULL);
  le = to_lecar_entry(e);
  e->time = cn->time;

  queue_push(&lecar->lecar_q[LECAR_CACHE], &le->lecar_list);
  le->list_id = LECAR_CACHE;

  heap_insert(&lecar->lfu, &le->lfu_list, freq);
}

int lecar_map(struct cache_nucleus *cn, oblock_t oblock, bool write,
              struct cache_nucleus_result *result) {
  struct lecar_policy *lecar = to_lecar_policy(cn);
  struct entry *e = hash_lookup(&cn->ht, oblock);
  int r = 0;

  inc_ios(&cn->stats);

  result->op = CACHE_NUCLEUS_MISS;

  lecar_clean_up(lecar);

  if (in_cache(cn, e)) {
    lecar_hit(lecar, e, result);
  } else {
    lecar_miss(lecar, oblock, result);
  }

  // dirty work
  {
    struct entry *cached_entry = hash_lookup(&cn->ht, oblock);
    if (in_cache(cn, cached_entry)) {
      cached_entry->dirty = cached_entry->dirty || write;
    }
  }

  return r;
}

void lecar_destroy(struct cache_nucleus *cn) {
  struct lecar_policy *lecar = to_lecar_policy(cn);

  heap_exit(&lecar->lfu);

  epool_exit(&cn->cache_pool);
  epool_exit(&cn->meta_pool);
  policy_exit(cn);
  mem_free(lecar);
}

int lecar_init(struct lecar_policy *lecar, cblock_t cache_size,
               cblock_t meta_size) {
  int i;
  struct cache_nucleus *cn = &lecar->nucleus;

  cache_nucleus_init_api(cn, lecar_map, lecar_remove, lecar_remove_preserve,
                         lecar_insert, default_lookup, default_cblock_lookup,
                         default_remap, lecar_destroy, default_get_stats,
                         default_set_time, default_residency,
                         default_cache_is_full);

  queue_init(&lecar->lecar_q[LECAR_CACHE]);
  queue_init(&lecar->lecar_q[LECAR_HIST]);

  // NOTE: can be changed. i'm just putting in a placeholder seed
  prandom_init_seed(&lecar->random_state, 42);

  lecar->error_discount_rate = fixedpt_rconst(0.99950000000000000000);
  lecar->learning_rate = fixedpt_rconst(0.45000000000000000000);
  lecar->W[0] = FIXEDPT_ONE_HALF;
  lecar->W[1] = FIXEDPT_ONE_HALF;

  lecar->history_size = meta_size;

  epool_create(&cn->cache_pool, cache_size, struct lecar_entry, e);
  if (!cn->cache_pool.entries_begin || !cn->cache_pool.entries) {
    LOG_DEBUG("epool_create for cache_pool failed.");
    goto cache_epool_create_fail;
  }
  // TODO reduce this?
  epool_create(&cn->meta_pool, lecar->history_size, struct lecar_entry, e);
  if (!cn->meta_pool.entries_begin || !cn->meta_pool.entries) {
    LOG_DEBUG("epool_create for meta_pool failed.");
    goto meta_epool_create_fail;
  }

  // TODO reduce this?
  if (heap_init(&lecar->lfu, 3 * cache_size)) {
    LOG_DEBUG("heap_init failed.");
    goto heap_init_fail;
  }

  heap_set_compare(&lecar->lfu, lecar_compare);

  if (policy_init(cn, cache_size, meta_size)) {
    LOG_DEBUG("policy_init failed");
    goto policy_init_fail;
  }

  return 0;

policy_init_fail:
  heap_exit(&lecar->lfu);
heap_init_fail:
  epool_exit(&cn->meta_pool);
meta_epool_create_fail:
  epool_exit(&cn->cache_pool);
cache_epool_create_fail:
  return -ENOSPC;
}

struct cache_nucleus *lecar_create(cblock_t cache_size, cblock_t meta_size) {
  int r;
  struct cache_nucleus *cn;
  struct lecar_policy *lecar = mem_alloc(sizeof(*lecar));
  if (lecar == NULL) {
    LOG_DEBUG("mem_alloc failed");
    goto lecar_create_fail;
  }

  cn = &lecar->nucleus;

  r = lecar_init(lecar, cache_size, meta_size);
  if (r) {
    LOG_DEBUG("lecar init failed. error %d", r);
    goto lecar_init_fail;
  }

  return cn;

lecar_init_fail:
  lecar_destroy(cn);
lecar_create_fail:
  return NULL;
}
