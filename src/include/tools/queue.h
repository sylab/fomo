#ifndef INCLUDE_TOOLS_QUEUE_H
#define INCLUDE_TOOLS_QUEUE_H

#include "common.h"
#include "kernel/list.h"

/** General queue implementation
 */
struct queue {
  struct list_head queue; ///< Head of the queue
  unsigned len;           ///< Length of the queue
};

/** General queue entry (or head, in reference to list_head of Linux's
 * linked-list implementation)
 */
struct queue_head {
  struct list_head list;      ///< Keeps place in queue
  struct queue *parent_queue; ///< Knows which queue it's in (for length
                              ///< altering purposes)
};

/** Initialize the queue
 *
 * \warning Does not reinitialize any items in the queue if re-initializing a
 *          non-empty queue
 */
static void queue_init(struct queue *q) {
  INIT_LIST_HEAD(&q->queue);
  q->len = 0;
}

/** Initialize a queue_head
 */
static void queue_head_init(struct queue_head *qh) {
  INIT_LIST_HEAD(&qh->list);
  qh->parent_queue = NULL;
}

/** Is the queue empty?
 * \return true if queue is empty, false otherwise
 */
static bool queue_empty(struct queue *q) { return list_empty(&q->queue); }

/** Push an item onto the queue
 * \note The item is pushed onto the tail of the queue
 */
static void queue_push(struct queue *q, struct queue_head *item) {
  item->parent_queue = q;
  list_add_tail(&item->list, &q->queue);
  ++q->len;
}

/** Remove an item from its queue
 */
static void queue_remove(struct queue_head *item) {
  struct queue *q;

  q = item->parent_queue;

  if (q) {
    struct list_head *lh;

    lh = &item->list;
    list_del(lh);

    --q->len;
    item->parent_queue = NULL;
  }
}

/** Replace an item in the queue with another
 */
static void queue_swap(struct queue_head *item_old,
                       struct queue_head *item_new) {
  struct list_head old_tmp;
  struct list_head new_tmp;
  struct queue *old_queue = item_old->parent_queue;
  struct queue *new_queue = item_new->parent_queue;

  // add temp spot holders
  list_add(&old_tmp, &item_old->list);
  list_add(&new_tmp, &item_new->list);

  // remove originals
  list_del(&item_old->list);
  list_del(&item_new->list);

  // add them after the opposite temp spot holder
  list_add(&item_new->list, &old_tmp);
  list_add(&item_old->list, &new_tmp);

  // remove temp spot holders
  list_del(&old_tmp);
  list_del(&new_tmp);

  // let them know they've swapped queues
  item_old->parent_queue = new_queue;
  item_new->parent_queue = old_queue;
}

/** Peek the item at the head of the queue
 * \return Item at the head of the queue or NULL if queue was empty
 */
static struct queue_head *queue_peek(struct queue *q) {
  return list_first_entry_or_null(&q->queue, struct queue_head, list);
}

/** Pop the item at the head of the queue
 * \return Popped item or NULL if queue was empty
 */
static struct queue_head *queue_pop(struct queue *q) {
  struct queue_head *item;

  item = queue_peek(q);
  if (item) {
    queue_remove(item);
  }

  return item;
}

/** The length of the queue
 */
static unsigned queue_length(struct queue *q) { return q->len; }

/** Check if item is in the given queue
 */
static bool in_queue(struct queue *q, struct queue_head *item) {
  return q == item->parent_queue;
}

#endif /* INCLUDE_TOOLS_QUEUE_H */
