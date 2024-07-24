#include "include/writeback_tracker.h"
#include "unity/unity.h"

void test_wb_init(void) {
  struct writeback_tracker wb;
  wb_init(&wb);
  TEST_ASSERT_EQUAL_UINT(0, queue_length(&wb.clean));
  TEST_ASSERT_EQUAL_UINT(0, queue_length(&wb.dirty));
  TEST_ASSERT_EQUAL_UINT(0, queue_length(&wb.meta));
}

void test_wb_push_cache_clean(void) {
  struct entry e;
  struct writeback_tracker wb;
  wb_init(&wb);

  e.dirty = false;
  wb_push_cache(&wb, &e);
  TEST_ASSERT_EQUAL_UINT(1, queue_length(&wb.clean));
  TEST_ASSERT_EQUAL_UINT(0, queue_length(&wb.dirty));
  TEST_ASSERT_EQUAL_UINT(0, queue_length(&wb.meta));
}

void test_wb_push_cache_dirty(void) {
  struct entry e;
  struct writeback_tracker wb;
  wb_init(&wb);

  e.dirty = true;
  wb_push_cache(&wb, &e);
  TEST_ASSERT_EQUAL_UINT(0, queue_length(&wb.clean));
  TEST_ASSERT_EQUAL_UINT(1, queue_length(&wb.dirty));
  TEST_ASSERT_EQUAL_UINT(0, queue_length(&wb.meta));
}

void test_wb_push_meta(void) {
  struct entry e;
  struct writeback_tracker wb;
  wb_init(&wb);

  wb_push_meta(&wb, &e);
  TEST_ASSERT_EQUAL_UINT(0, queue_length(&wb.clean));
  TEST_ASSERT_EQUAL_UINT(0, queue_length(&wb.dirty));
  TEST_ASSERT_EQUAL_UINT(1, queue_length(&wb.meta));
}

void test_wb_remove_clean(void) {
  struct entry e;
  struct writeback_tracker wb;
  wb_init(&wb);

  e.dirty = false;
  wb_push_cache(&wb, &e);
  TEST_ASSERT_EQUAL_UINT(1, queue_length(&wb.clean));
  wb_remove(&e);
  TEST_ASSERT_EQUAL_UINT(0, queue_length(&wb.clean));
}

void test_wb_remove_dirty(void) {
  struct entry e;
  struct writeback_tracker wb;
  wb_init(&wb);

  e.dirty = true;
  wb_push_cache(&wb, &e);
  TEST_ASSERT_EQUAL_UINT(1, queue_length(&wb.dirty));
  wb_remove(&e);
  TEST_ASSERT_EQUAL_UINT(0, queue_length(&wb.dirty));
}

void test_wb_remove_meta(void) {
  struct entry e;
  struct writeback_tracker wb;
  wb_init(&wb);

  wb_push_meta(&wb, &e);
  TEST_ASSERT_EQUAL_UINT(1, queue_length(&wb.meta));
  wb_remove(&e);
  TEST_ASSERT_EQUAL_UINT(0, queue_length(&wb.meta));
}

void test_wb_pop_dirty(void) {
  struct entry *popped;
  struct entry e, f, g;
  struct writeback_tracker wb;
  wb_init(&wb);

  e.dirty = true;
  f.dirty = true;
  g.dirty = false;
  wb_push_cache(&wb, &e);
  wb_push_cache(&wb, &f);
  wb_push_cache(&wb, &g);
  TEST_ASSERT_EQUAL_UINT(1, queue_length(&wb.clean));
  TEST_ASSERT_EQUAL_UINT(2, queue_length(&wb.dirty));

  popped = wb_pop_dirty(&wb);
  TEST_ASSERT_NOT_NULL(popped);
  TEST_ASSERT_TRUE(popped->dirty);
  TEST_ASSERT_EQUAL(&e, popped);
  TEST_ASSERT_EQUAL_UINT(1, queue_length(&wb.clean));
  TEST_ASSERT_EQUAL_UINT(1, queue_length(&wb.dirty));

  popped->dirty = false;
  wb_push_cache(&wb, popped);
  TEST_ASSERT_EQUAL_UINT(2, queue_length(&wb.clean));
  TEST_ASSERT_EQUAL_UINT(1, queue_length(&wb.dirty));

  popped = wb_pop_dirty(&wb);
  TEST_ASSERT_NOT_NULL(popped);
  TEST_ASSERT_TRUE(popped->dirty);
  TEST_ASSERT_EQUAL(&f, popped);
  TEST_ASSERT_EQUAL_UINT(2, queue_length(&wb.clean));
  TEST_ASSERT_EQUAL_UINT(0, queue_length(&wb.dirty));

  popped = wb_pop_dirty(&wb);
  TEST_ASSERT_NULL(popped);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_wb_init);
  RUN_TEST(test_wb_push_cache_clean);
  RUN_TEST(test_wb_push_cache_dirty);
  RUN_TEST(test_wb_push_meta);
  RUN_TEST(test_wb_remove_clean);
  RUN_TEST(test_wb_remove_dirty);
  RUN_TEST(test_wb_remove_meta);
  RUN_TEST(test_wb_pop_dirty);
  return UNITY_END();
}
