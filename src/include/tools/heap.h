#ifndef INCLUDE_TOOLS_HEAP_H
#define INCLUDE_TOOLS_HEAP_H

#include "common.h"

/** heap_head:
 * Added to a struct to have a part that's tracked by the heap,
 * with the key managed by the containing struct, and index
 * typically managed by the heap
 * NOTE: keys are unsigned since the heap is mostly designed for
 *       using key as a frequency
 */
struct heap_head {
  unsigned key;
  unsigned index;
};

typedef int (*heap_compare_f)(struct heap_head *hh_a, struct heap_head *hh_b);

/** heap:
 * A traditional min-heap
 *
 * heap_arr  Array of heap_head pointers
 *           Allows for easy access and swaps of pointers
 *           for arranging heap_head's in the heap
 * capacity  How big the heap is
 * size      How many items are in the heap
 * compare   A function that compares between two heap_heads, helps
 *           with getting the heap to work and can be customized for different
 *           requirements while having a default behavior (heap_default_compare)
 */
struct heap {
  struct heap_head **heap_arr;
  unsigned capacity;
  unsigned size;
  heap_compare_f compare;
};

/** heap_default_compare:
 * Compares the only thing to compare between heap_head's: the keys
 * Hints at how to design custom compare functions:
 *   if  a < b: return a value < 0 (such as -1)
 *   if  a > b: return a value > 0 (such as  1)
 *   if a == b: return 0
 * NOTE: Since the keys are unsigned, it's possible to have some improper
 *       evaluations should we only return a->key - b->key.
 *       Due to this, we use if logic to get around this possibility
 */
static int heap_default_compare(struct heap_head *a, struct heap_head *b) {
  if (a->key < b->key) {
    return -1;
  }
  if (a->key > b->key) {
    return 1;
  }
  return 0;
}

/** heap_init:
 * Initializes an empty heap with a certain capacity.
 * Sets the default compare function.
 */
static int heap_init(struct heap *heap, unsigned capacity) {
  heap->heap_arr = mem_alloc(sizeof(*heap->heap_arr) * capacity);
  if (!heap->heap_arr)
    return -ENOMEM;

  heap->capacity = capacity;
  heap->size = 0;

  heap->compare = heap_default_compare;

  return 0;
}

/** heap_exit:
 * "Exit" the heap to free up held memory.
 */
static void heap_exit(struct heap *heap) {
  heap->capacity = 0;
  heap->size = 0;
  mem_free(heap->heap_arr);
}

/** heap_full:
 * returns true if heap is full, otherwise returns false
 */
static bool heap_full(struct heap *heap) {
  return heap->size == heap->capacity;
}

static struct heap_head *heap__parent(struct heap *heap,
                                      struct heap_head *child) {
  if (child->index > 0) {
    return heap->heap_arr[(child->index - 1) / 2];
  }
  return NULL;
}

static struct heap_head *heap__left(struct heap *heap,
                                    struct heap_head *parent) {
  unsigned index = 2 * parent->index + 1;
  if (index < heap->size) {
    return heap->heap_arr[index];
  }
  return NULL;
}

static struct heap_head *heap__right(struct heap *heap,
                                     struct heap_head *parent) {
  unsigned index = 2 * parent->index + 2;
  if (index < heap->size) {
    return heap->heap_arr[index];
  }
  return NULL;
}

/** heap_swap:
 * Swap two items in the heap
 */
static void heap_swap(struct heap *heap, struct heap_head *a,
                      struct heap_head *b) {
  unsigned a_index = a->index;

  a->index = b->index;
  b->index = a_index;

  heap->heap_arr[a->index] = a;
  heap->heap_arr[b->index] = b;
}

static void heap_decrease_key(struct heap *heap, struct heap_head *h,
                              unsigned key) {
  struct heap_head *parent = heap__parent(heap, h);

  LOG_ASSERT(key <= h->key);
  h->key = key;

  // move h up the heap as needed
  while (h->index > 0 && heap->compare(h, parent) < 0) {
    heap_swap(heap, h, parent);
    parent = heap__parent(heap, h);
  }
}

/** heap_insert:
 * Insert the following heap_head into the heap with the following key
 * NOTE: it has been considered to ONLY have externally managed key for this,
 *       but it's not much to also have it here
 */
static void heap_insert(struct heap *heap, struct heap_head *h, unsigned key) {
  unsigned index = heap->size;
  LOG_ASSERT(!heap_full(heap));

  ++heap->size;
  h->key = key;
  h->index = index;
  heap->heap_arr[index] = h;

  heap_decrease_key(heap, h, key);
}

static void heapify(struct heap *heap, unsigned index) {
  struct heap_head *i;
  struct heap_head *l;
  struct heap_head *r;
  struct heap_head *min;

  if (index >= heap->size)
    return;

  i = heap->heap_arr[index];
  l = heap__left(heap, i);
  r = heap__right(heap, i);
  min = i;

  if (l != NULL && heap->compare(l, min) < 0) {
    min = l;
  }
  if (r != NULL && heap->compare(r, min) < 0) {
    min = r;
  }
  if (min != i) {
    heap_swap(heap, i, min);
    heapify(heap, i->index);
  }
}

static void heap_build(struct heap *heap) {
  struct heap_head *h;

  if (heap->size == 0)
    return;

  h = heap->heap_arr[heap->size - 1];
  while (h != NULL) {
    heapify(heap, h->index);
    h = heap__parent(heap, h);
  }
}

static void heap_set_compare(struct heap *heap, heap_compare_f compare) {
  heap->compare = compare;
  heap_build(heap);
}

static void heap_delete(struct heap *heap, struct heap_head *h) {
  struct heap_head *last = heap->heap_arr[heap->size - 1];

  heap_swap(heap, h, last);

  // remove h from heap
  heap->heap_arr[h->index] = NULL;
  --heap->size;

  // fix up heap
  if (heap->compare(last, h) < 0) {
    heap_decrease_key(heap, last, last->key);
  } else {
    heapify(heap, last->index);
  }
}

static struct heap_head *heap_min(struct heap *heap) {
  struct heap_head *min;

  if (heap->size == 0)
    return NULL;

  min = heap->heap_arr[0];

  return min;
}

static struct heap_head *heap_min_pop(struct heap *heap) {
  struct heap_head *min = heap_min(heap);

  if (min) {
    heap_delete(heap, min);
  }

  return min;
}

static bool in_heap(struct heap *heap, struct heap_head *h) {
  if (h == NULL)
    return false;

  if (h->index >= heap->size)
    return false;

  return heap->heap_arr[h->index] == h;
}

#endif /* INCLUDE_TOOLS_HEAP_H */
