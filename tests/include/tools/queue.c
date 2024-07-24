#include "../src/include/tools/queue.h"
#include "unity/unity.h"

void test_queue_init(void) {
  struct queue q;
  queue_init(&q);
  TEST_ASSERT_EQUAL_UINT(queue_length(&q), 0);
  TEST_ASSERT(queue_empty(&q));
}

void test_queue_push(void) {
  struct queue q;
  struct queue_head elt;
  queue_init(&q);
  queue_push(&q, &elt);
  TEST_ASSERT_EQUAL_UINT(queue_length(&q), 1);
  TEST_ASSERT(!queue_empty(&q));
  TEST_ASSERT_EQUAL(q.queue.prev, &elt.list);
  TEST_ASSERT_EQUAL(elt.parent_queue, &q);
}

void test_queue_remove(void) {
  struct queue q;
  struct queue_head elt;
  queue_init(&q);
  queue_push(&q, &elt);

  queue_remove(&elt);
  TEST_ASSERT_EQUAL_UINT(queue_length(&q), 0);
  TEST_ASSERT(queue_empty(&q));
  TEST_ASSERT_NULL(elt.parent_queue);
}

void test_queue_pop(void) {
  struct queue q;
  struct queue_head arr[5];
  struct queue_head *popped;
  int len = 5;
  queue_init(&q);

  popped = queue_pop(&q);
  TEST_ASSERT_NULL(popped);

  for (int i = 0; i < len; ++i) {
    queue_push(&q, &arr[i]);
  }

  for (int i = 0; i < len; ++i) {
    TEST_ASSERT_EQUAL_UINT(queue_length(&q), len - i);
    popped = queue_pop(&q);
    TEST_ASSERT_EQUAL_UINT(queue_length(&q), len - 1 - i);
    TEST_ASSERT_EQUAL(popped, &arr[i]);
  }
}

void test_queue_in_queue(void) {
  struct queue q;
  struct queue_head elt;
  queue_init(&q);
  queue_push(&q, &elt);

  TEST_ASSERT_TRUE(in_queue(&q, &elt));
}

void test_queue_replace(void) {
  struct queue q;
  struct queue_head elt;
  struct queue_head filler;
  struct queue_head replace;
  queue_init(&q);
  queue_push(&q, &elt);
  queue_push(&q, &filler);
  queue_push(&q, &replace);

  queue_swap(&elt, &replace);

  TEST_ASSERT_EQUAL(&replace, queue_pop(&q));
  TEST_ASSERT_EQUAL(&filler, queue_pop(&q));
  TEST_ASSERT_EQUAL(&elt, queue_pop(&q));
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_queue_init);
  RUN_TEST(test_queue_push);
  RUN_TEST(test_queue_remove);
  RUN_TEST(test_queue_pop);
  RUN_TEST(test_queue_in_queue);
  RUN_TEST(test_queue_replace);
  return UNITY_END();
}
