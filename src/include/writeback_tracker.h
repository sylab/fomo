#ifndef WRITEBACK_TRACKER_H
#define WRITEBACK_TRACKER_H

#include "entry.h"
#include "tools/queue.h"

/** Tracks dirty, clean, and meta entries for writeback
 *
 * A cache in writeback mode needs to know which entries are more up-to-date
 * than the same entry in the origin device. When a dirty entry is evicted, it
 * must be written back (get it?) to update the origin device since the cache
 * will no longer keep the most up-to-date version of the entry.
 *
 * clean - Entries that match their origin device counterpart
 * dirty - Entries that are more up-to-date thatn their origin device
 *         counterpart
 * meta - Entries that are not in the cache, but still tracked
 */
struct writeback_tracker {
  struct queue clean;
  struct queue dirty;
  struct queue meta;
};

static void wb_init(struct writeback_tracker *wb_tracker) {
  queue_init(&wb_tracker->clean);
  queue_init(&wb_tracker->dirty);
  queue_init(&wb_tracker->meta);
}

/** Push entry to cache queue (dirty or clean based on entry)
 */
static void wb_push_cache(struct writeback_tracker *wb_tracker,
                          struct entry *e) {
  queue_push(e->dirty ? &wb_tracker->dirty : &wb_tracker->clean, &e->wb_list);
}

/** Push entry to meta queue
 */
static void wb_push_meta(struct writeback_tracker *wb_tracker,
                         struct entry *e) {
  queue_push(&wb_tracker->meta, &e->wb_list);
}

/** Remove entry from writeback queue
 */
static void wb_remove(struct entry *e) { queue_remove(&e->wb_list); }

/** Remove and retrieve least recently used dirty entry from queue
 */
static struct entry *wb_pop_dirty(struct writeback_tracker *wb_tracker) {
  struct queue_head *h = queue_pop(&wb_tracker->dirty);
  if (!h)
    return NULL;

  return container_of(h, struct entry, wb_list);
}

#endif /* WRITEBACK_TRACKER_H */
