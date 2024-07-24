#ifndef SIM_SIM_FREQ_H
#define SIM_SIM_FREQ_H

#include "cache_nucleus.h"
#include "common.h"
#include "entry.h"
#include "tools/hashtable.h"

struct freq_tracker_entry {
  struct entry e;
  unsigned freq;
};

struct freq_tracker {
  struct hashtable ht;
  struct entry_pool cache_pool;
  unsigned min_freq;
  unsigned count;
};

static struct freq_tracker_entry *to_freq_tracker_entry(struct entry *e) {
  return container_of(e, struct freq_tracker_entry, e);
}

static void freq_tracker_print_stats(struct freq_tracker *f_tracker,
                                     unsigned sampling_rate) {
  LOG_PRINT("%u", f_tracker->count * sampling_rate);
}

static void freq_tracker_update(struct freq_tracker *f_tracker, oblock_t oblock,
                                struct cache_nucleus_result *result) {
  struct freq_tracker_entry *fe;
  struct entry *e;

  switch (result->op) {
  case CACHE_NUCLEUS_HIT:
    e = hash_lookup(&f_tracker->ht, oblock);
    fe = to_freq_tracker_entry(e);
    ++fe->freq;
    if (fe->freq == f_tracker->min_freq) {
      ++f_tracker->count;
    }
    break;

  case CACHE_NUCLEUS_FILTER:
  case CACHE_NUCLEUS_FAIL:
    break;

  case CACHE_NUCLEUS_REPLACE:
    e = hash_lookup(&f_tracker->ht, result->old_oblock);
    hash_remove(e);
    free_entry(&f_tracker->cache_pool, e);

  case CACHE_NUCLEUS_NEW:
    e = alloc_entry(&f_tracker->cache_pool);
    LOG_ASSERT(e != NULL);

    e->oblock = oblock;
    fe = to_freq_tracker_entry(e);
    fe->freq = 1;
    hash_insert(&f_tracker->ht, e);

    break;
  }
}

static void freq_tracker_exit(struct freq_tracker *f_tracker) {
  hash_exit(&f_tracker->ht);
  epool_exit(&f_tracker->cache_pool);
}

static int freq_tracker_init(struct freq_tracker *f_tracker,
                             cblock_t cache_size, unsigned min_freq) {
  int r;
  r = ht_init(&f_tracker->ht, cache_size);
  if (r) {
    LOG_DEBUG("ht_init failed");
    goto ht_init_fail;
  }

  epool_create(&f_tracker->cache_pool, cache_size, struct freq_tracker_entry,
               e);
  if (!f_tracker->cache_pool.entries_begin || !f_tracker->cache_pool.entries) {
    LOG_DEBUG("epool_create for cache_pool failed");
    r = -ENOSPC;
    goto cache_epool_create_fail;
  }
  epool_init(&f_tracker->cache_pool, 0);

  f_tracker->min_freq = min_freq;
  f_tracker->count = 0;

  return 0;

cache_epool_create_fail:
  hash_exit(&f_tracker->ht);
ht_init_fail:
  return r;
}

#endif /* SIM_SIM_FREQ_H */
