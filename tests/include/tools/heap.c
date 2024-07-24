#include "tools/heap.h"
#include "unity/unity.h"

const unsigned HEAP_CAPACITY = 5;

void test_heap_init(void) {
  struct heap heap;

  heap_init(&heap, HEAP_CAPACITY);
  TEST_ASSERT_NOT_NULL(heap.heap_arr);
  TEST_ASSERT_NULL(heap.heap_arr[0]);
  TEST_ASSERT_EQUAL_UINT(HEAP_CAPACITY, heap.capacity);
  TEST_ASSERT_EQUAL_UINT(0, heap.size);

  heap_exit(&heap);
}

void test_heap_exit(void) {
  struct heap heap;
  heap_init(&heap, HEAP_CAPACITY);
  TEST_ASSERT_EQUAL_UINT(HEAP_CAPACITY, heap.capacity);

  heap_exit(&heap);
  TEST_ASSERT_EQUAL_UINT(0, heap.capacity);
}

void test_heap_full(void) {
  struct heap heap;
  heap_init(&heap, HEAP_CAPACITY);

  // be empty
  TEST_ASSERT_FALSE(heap_full(&heap));

  // pretend to almost be full
  heap.size = HEAP_CAPACITY - 1;
  TEST_ASSERT_FALSE(heap_full(&heap));

  // pretend to be full
  heap.size = HEAP_CAPACITY;
  TEST_ASSERT_TRUE(heap_full(&heap));

  heap_exit(&heap);
}

void test_heap_swap(void) {
  struct heap heap;
  unsigned a_index = 0;
  unsigned b_index = HEAP_CAPACITY - 1;
  struct heap_head a = {.key = 1, .index = a_index};
  struct heap_head b = {.key = 2, .index = b_index};
  heap_init(&heap, HEAP_CAPACITY);

  // put a and be where they belong first
  heap.heap_arr[a_index] = &a;
  heap.heap_arr[b_index] = &b;
  TEST_ASSERT_EQUAL(&a, heap.heap_arr[a_index]);
  TEST_ASSERT_EQUAL_UINT(a.key, heap.heap_arr[a_index]->key);
  TEST_ASSERT_EQUAL(&b, heap.heap_arr[b_index]);
  TEST_ASSERT_EQUAL_UINT(b.key, heap.heap_arr[b_index]->key);

  heap_swap(&heap, heap.heap_arr[a_index], heap.heap_arr[b_index]);
  TEST_ASSERT_EQUAL(&b, heap.heap_arr[a_index]);
  TEST_ASSERT_EQUAL_UINT(b.key, heap.heap_arr[a_index]->key);
  TEST_ASSERT_EQUAL(&a, heap.heap_arr[b_index]);
  TEST_ASSERT_EQUAL_UINT(a.key, heap.heap_arr[b_index]->key);

  heap_exit(&heap);
}

void test_heap_decrease_key(void) {
  struct heap heap;
  struct heap_head a, b, c, d, e;
  heap_init(&heap, HEAP_CAPACITY);

  // make heap [ 1, 2, 4, 5, 3 ]
  heap_insert(&heap, &a, 5);
  heap_insert(&heap, &b, 4);
  heap_insert(&heap, &c, 3);
  heap_insert(&heap, &d, 2);
  heap_insert(&heap, &e, 1);

  TEST_ASSERT_EQUAL(&e, heap.heap_arr[0]);
  TEST_ASSERT_EQUAL(&d, heap.heap_arr[1]);
  TEST_ASSERT_EQUAL(&b, heap.heap_arr[2]);
  TEST_ASSERT_EQUAL(&a, heap.heap_arr[3]);
  TEST_ASSERT_EQUAL(&c, heap.heap_arr[4]);

  TEST_ASSERT_EQUAL_UINT(3, c.key);
  heap_decrease_key(&heap, &c, 0);
  TEST_ASSERT_EQUAL_UINT(0, c.key);

  TEST_ASSERT_EQUAL(&c, heap.heap_arr[0]);
  TEST_ASSERT_EQUAL(&e, heap.heap_arr[1]);
  TEST_ASSERT_EQUAL(&b, heap.heap_arr[2]);
  TEST_ASSERT_EQUAL(&a, heap.heap_arr[3]);
  TEST_ASSERT_EQUAL(&d, heap.heap_arr[4]);

  heap_exit(&heap);
}

void test_heap_insert(void) {
  struct heap heap;
  struct heap_head a, b, c, d, e;
  heap_init(&heap, HEAP_CAPACITY);

  heap_insert(&heap, &a, 5);
  TEST_ASSERT_EQUAL_UINT(1, heap.size);
  TEST_ASSERT_EQUAL(&a, heap.heap_arr[0]);
  TEST_ASSERT_EQUAL_UINT(5, heap.heap_arr[0]->key);
  TEST_ASSERT_EQUAL_UINT(0, heap.heap_arr[0]->index);
  TEST_ASSERT_EQUAL_UINT(5, a.key);
  TEST_ASSERT_EQUAL_UINT(0, a.index);

  heap_insert(&heap, &b, 4);
  TEST_ASSERT_EQUAL_UINT(2, heap.size);
  TEST_ASSERT_EQUAL(&b, heap.heap_arr[0]);
  TEST_ASSERT_EQUAL(&a, heap.heap_arr[1]);
  TEST_ASSERT_EQUAL_UINT(5, a.key);
  TEST_ASSERT_EQUAL_UINT(1, a.index);

  heap_insert(&heap, &c, 3);
  heap_insert(&heap, &d, 2);
  heap_insert(&heap, &e, 1);
  TEST_ASSERT_EQUAL_UINT(5, heap.size);
  TEST_ASSERT_EQUAL(&e, heap.heap_arr[0]);
  TEST_ASSERT_EQUAL(&d, heap.heap_arr[1]);
  TEST_ASSERT_EQUAL(&b, heap.heap_arr[2]);
  TEST_ASSERT_EQUAL(&a, heap.heap_arr[3]);
  TEST_ASSERT_EQUAL(&c, heap.heap_arr[4]);

  heap_exit(&heap);
}

void test_heapify(void) {
  struct heap heap;
  struct heap_head a, b, c, d, e;
  heap_init(&heap, HEAP_CAPACITY);

  heap_insert(&heap, &a, 1);
  heap_insert(&heap, &b, 2);
  heap_insert(&heap, &c, 3);
  heap_insert(&heap, &d, 4);
  heap_insert(&heap, &e, 5);

  // make heap [ 5, 4, 3, 2, 1 ]
  heap.heap_arr[0]->key = 5;
  heap.heap_arr[1]->key = 4;
  heap.heap_arr[2]->key = 3;
  heap.heap_arr[3]->key = 2;
  heap.heap_arr[4]->key = 1;

  TEST_ASSERT_EQUAL(&a, heap.heap_arr[0]);
  TEST_ASSERT_EQUAL(&b, heap.heap_arr[1]);
  TEST_ASSERT_EQUAL(&c, heap.heap_arr[2]);
  TEST_ASSERT_EQUAL(&d, heap.heap_arr[3]);
  TEST_ASSERT_EQUAL(&e, heap.heap_arr[4]);

  // heapify part of it
  heapify(&heap, 1);
  TEST_ASSERT_EQUAL(&a, heap.heap_arr[0]);
  TEST_ASSERT_EQUAL(&e, heap.heap_arr[1]);
  TEST_ASSERT_EQUAL(&c, heap.heap_arr[2]);
  TEST_ASSERT_EQUAL(&d, heap.heap_arr[3]);
  TEST_ASSERT_EQUAL(&b, heap.heap_arr[4]);

  // heapify all of it
  heapify(&heap, 0);
  TEST_ASSERT_EQUAL(&e, heap.heap_arr[0]);
  TEST_ASSERT_EQUAL(&d, heap.heap_arr[1]);
  TEST_ASSERT_EQUAL(&c, heap.heap_arr[2]);
  TEST_ASSERT_EQUAL(&a, heap.heap_arr[3]);
  TEST_ASSERT_EQUAL(&b, heap.heap_arr[4]);

  heap_exit(&heap);
}

void test_heap_delete(void) {
  struct heap heap;
  struct heap_head a, b, c, d, e;
  heap_init(&heap, HEAP_CAPACITY);

  heap_insert(&heap, &a, 5);
  TEST_ASSERT_EQUAL(&a, heap.heap_arr[0]);
  TEST_ASSERT_EQUAL_UINT(5, heap.heap_arr[0]->key);
  TEST_ASSERT_EQUAL_UINT(1, heap.size);
  heap_delete(&heap, &a);
  TEST_ASSERT_NULL(heap.heap_arr[0]);
  TEST_ASSERT_EQUAL_UINT(0, heap.size);

  heap_insert(&heap, &a, 5);
  heap_insert(&heap, &b, 4);
  heap_insert(&heap, &c, 3);
  heap_insert(&heap, &d, 2);
  heap_insert(&heap, &e, 1);

  // delete middle one
  // [ 1, 2, 4, 5, 3 ] -> [ 1, 3, 4, 5 ]
  TEST_ASSERT_EQUAL(&d, heap.heap_arr[1]);
  TEST_ASSERT_EQUAL_UINT(2, heap.heap_arr[1]->key);
  TEST_ASSERT_EQUAL_UINT(5, heap.size);
  heap_delete(&heap, &d);
  TEST_ASSERT_EQUAL(&c, heap.heap_arr[1]);
  TEST_ASSERT_EQUAL_UINT(2, d.key);
  TEST_ASSERT_EQUAL_UINT(4, heap.size);

  // delete top one
  // [ 1, 3, 4, 5 ] -> [ 3, 5, 4 ]
  heap_delete(&heap, &e);
  TEST_ASSERT_EQUAL(&c, heap.heap_arr[0]);
  TEST_ASSERT_EQUAL(&a, heap.heap_arr[1]);
  TEST_ASSERT_EQUAL(&b, heap.heap_arr[2]);
  TEST_ASSERT_EQUAL_UINT(1, e.key);
  TEST_ASSERT_EQUAL_UINT(3, heap.size);

  heap_exit(&heap);
}

void test_heap_min(void) {
  struct heap heap;
  struct heap_head a, b, c, d, e;
  heap_init(&heap, HEAP_CAPACITY);

  TEST_ASSERT_NULL(heap_min(&heap));

  heap_insert(&heap, &a, 5);
  TEST_ASSERT_EQUAL(&a, heap_min(&heap));
  heap_insert(&heap, &b, 4);
  TEST_ASSERT_EQUAL(&b, heap_min(&heap));
  heap_insert(&heap, &c, 3);
  TEST_ASSERT_EQUAL(&c, heap_min(&heap));
  heap_insert(&heap, &d, 2);
  TEST_ASSERT_EQUAL(&d, heap_min(&heap));
  heap_insert(&heap, &e, 1);
  TEST_ASSERT_EQUAL(&e, heap_min(&heap));

  heap_exit(&heap);
}

void test_heap_min_pop(void) {
  struct heap heap;
  struct heap_head a, b, c, d, e;
  heap_init(&heap, HEAP_CAPACITY);
  heap_insert(&heap, &a, 5);
  heap_insert(&heap, &b, 4);
  heap_insert(&heap, &c, 3);
  heap_insert(&heap, &d, 2);
  heap_insert(&heap, &e, 1);

  TEST_ASSERT_EQUAL_UINT(5, heap.size);
  TEST_ASSERT_EQUAL(&e, heap_min_pop(&heap));
  TEST_ASSERT_EQUAL_UINT(4, heap.size);
  TEST_ASSERT_EQUAL(&d, heap_min_pop(&heap));
  TEST_ASSERT_EQUAL_UINT(3, heap.size);
  TEST_ASSERT_EQUAL(&c, heap_min_pop(&heap));
  TEST_ASSERT_EQUAL_UINT(2, heap.size);
  TEST_ASSERT_EQUAL(&b, heap_min_pop(&heap));
  TEST_ASSERT_EQUAL_UINT(1, heap.size);
  TEST_ASSERT_EQUAL(&a, heap_min_pop(&heap));
  TEST_ASSERT_EQUAL_UINT(0, heap.size);
  TEST_ASSERT_EQUAL(NULL, heap_min_pop(&heap));
  TEST_ASSERT_EQUAL_UINT(0, heap.size);

  heap_exit(&heap);
}

void test_heap_build(void) {
  struct heap heap;
  struct heap_head a, b, c, d, e;
  heap_init(&heap, HEAP_CAPACITY);
  heap_insert(&heap, &a, 5);
  heap_insert(&heap, &b, 4);
  heap_insert(&heap, &c, 3);
  heap_insert(&heap, &d, 2);
  heap_insert(&heap, &e, 1);

  heap.heap_arr[0]->key = 5;
  heap.heap_arr[1]->key = 2;
  heap.heap_arr[2]->key = 3;
  heap.heap_arr[3]->key = 1;
  heap.heap_arr[4]->key = 4;

  // order into heap now
  heap_build(&heap);

  TEST_ASSERT_EQUAL(1, heap.heap_arr[0]->key);
  TEST_ASSERT_EQUAL(2, heap.heap_arr[1]->key);
  TEST_ASSERT_EQUAL(3, heap.heap_arr[2]->key);
  TEST_ASSERT_EQUAL(5, heap.heap_arr[3]->key);
  TEST_ASSERT_EQUAL(4, heap.heap_arr[4]->key);

  TEST_ASSERT_EQUAL(1, heap_min_pop(&heap)->key);
  TEST_ASSERT_EQUAL(2, heap_min_pop(&heap)->key);
  TEST_ASSERT_EQUAL(3, heap_min_pop(&heap)->key);
  TEST_ASSERT_EQUAL(4, heap_min_pop(&heap)->key);
  TEST_ASSERT_EQUAL(5, heap_min_pop(&heap)->key);
}

int test_heap_compare(struct heap_head *a, struct heap_head *b) {
  if (a->key > b->key) {
    return -1;
  }
  if (a->key < b->key) {
    return 1;
  }
  return 0;
}

void test_heap_set_compare(void) {
  struct heap heap;
  struct heap_head a, b, c, d, e;
  heap_init(&heap, HEAP_CAPACITY);
  heap_insert(&heap, &a, 5);
  heap_insert(&heap, &b, 4);
  heap_insert(&heap, &c, 3);
  heap_insert(&heap, &d, 2);
  heap_insert(&heap, &e, 1);

  heap_set_compare(&heap, test_heap_compare);

  TEST_ASSERT_EQUAL(5, heap.heap_arr[0]->key);
  TEST_ASSERT_EQUAL(3, heap.heap_arr[1]->key);
  TEST_ASSERT_EQUAL(4, heap.heap_arr[2]->key);
  TEST_ASSERT_EQUAL(2, heap.heap_arr[3]->key);
  TEST_ASSERT_EQUAL(1, heap.heap_arr[4]->key);

  TEST_ASSERT_EQUAL(5, heap_min_pop(&heap)->key);
  TEST_ASSERT_EQUAL(4, heap_min_pop(&heap)->key);
  TEST_ASSERT_EQUAL(3, heap_min_pop(&heap)->key);
  TEST_ASSERT_EQUAL(2, heap_min_pop(&heap)->key);
  TEST_ASSERT_EQUAL(1, heap_min_pop(&heap)->key);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_heap_init);
  RUN_TEST(test_heap_exit);
  RUN_TEST(test_heap_full);
  RUN_TEST(test_heap_swap);
  RUN_TEST(test_heap_decrease_key);
  RUN_TEST(test_heap_insert);
  RUN_TEST(test_heapify);
  RUN_TEST(test_heap_delete);
  RUN_TEST(test_heap_min);
  RUN_TEST(test_heap_min_pop);
  RUN_TEST(test_heap_build);
  RUN_TEST(test_heap_set_compare);
  return UNITY_END();
}
