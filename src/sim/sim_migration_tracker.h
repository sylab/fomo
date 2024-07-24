#ifndef SIM_MIGRATION_TRACKER_H
#define SIM_MIGRATION_TRACKER_H

#include "common.h"
#include "entry.h"
#include "tools/hashtable.h"
#include "tools/queue.h"

struct migration_op {
  struct entry e;
  unsigned migrated_time;
  struct queue_head list;
};

struct migration_tracker {
  struct hashtable ht;

  struct entry_pool entry_pool;
  struct queue q;

  unsigned delay;
};

static struct migration_op *to_migration_op(struct entry *e) {
  return container_of(e, struct migration_op, e);
}

static void migration_remove(struct migration_tracker *m_tracker,
                             oblock_t oblock) {
  struct entry *e = hash_lookup(&m_tracker->ht, oblock);
  if (e != NULL) {
    struct migration_op *op = to_migration_op(e);
    op->migrated_time = 0;
    queue_remove(&op->list);
    hash_remove(e);
    free_entry(&m_tracker->entry_pool, e);
  }
}

static void migration_add(struct migration_tracker *m_tracker,
                          unsigned current_time, oblock_t oblock) {
  struct entry *e = hash_lookup(&m_tracker->ht, oblock);
  if (e == NULL) {
    struct migration_op *op;
    e = alloc_entry(&m_tracker->entry_pool);
    e->oblock = oblock;
    hash_insert(&m_tracker->ht, e);
    op = to_migration_op(e);
    op->migrated_time = current_time + m_tracker->delay;

    queue_push(&m_tracker->q, &op->list);
  }
}

static bool migration_next(struct migration_tracker *m_tracker,
                           unsigned current_time, oblock_t *oblock) {
  if (queue_length(&m_tracker->q) > 0) {
    struct queue_head *qh = queue_peek(&m_tracker->q);
    struct migration_op *next = container_of(qh, struct migration_op, list);
    if (next->migrated_time == current_time) {
      *oblock = next->e.oblock;
      migration_remove(m_tracker, next->e.oblock);
      return true;
    }
  }
  return false;
}

static void migration_tracker_exit(struct migration_tracker *m_tracker) {
  hash_exit(&m_tracker->ht);
  epool_exit(&m_tracker->entry_pool);
}

static int migration_tracker_init(struct migration_tracker *m_tracker,
                                  unsigned delay) {
  // TODO
  int r;

  r = ht_init(&m_tracker->ht, 2 * delay);
  if (r) {
    LOG_DEBUG("ht_init failed");
    r = -ENOSPC;
    goto ht_init_fail;
  }

  epool_create(&m_tracker->entry_pool, 2 * delay, struct migration_op, e);
  if (!m_tracker->entry_pool.entries_begin || !m_tracker->entry_pool.entries) {
    LOG_DEBUG("epool_create for entry_pool failed");
    r = -ENOSPC;
    goto entry_pool_create_fail;
  }
  epool_init(&m_tracker->entry_pool, 0);

  queue_init(&m_tracker->q);

  m_tracker->delay = delay;

  return 0;

entry_pool_create_fail:
  hash_exit(&m_tracker->ht);
ht_init_fail:
  return r;
}

#endif
