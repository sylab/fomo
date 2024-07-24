#include "unity/unity.h"
#include <assert.h>
#include <stdio.h>

#include "../src/include/entry.h"
#include "../src/include/tools/epool.h"

struct test_entry {
  int filler;
  struct entry e;
};

void test_epool_create(void) {
  int len = 5;
  struct entry_pool ep = {0};
  epool_create(&ep, len, struct test_entry, e);

  TEST_ASSERT(!epool_empty(&ep));
  TEST_ASSERT_EQUAL_UINT(ep.nr_entries, len);
  TEST_ASSERT_EQUAL(((struct test_entry *)ep.entries_begin) + len,
                    ep.entries_end);

  epool_exit(&ep);
}

void test_epool_at(void) {
  int len = 5;
  struct entry_pool ep = {0};
  epool_create(&ep, len, struct test_entry, e);
  for (int i = 0; i < len; ++i) {
    TEST_ASSERT_EQUAL(epool_at(&ep, i), ep.entries[i]);
  }
  TEST_ASSERT_NULL(epool_at(&ep, len));

  epool_exit(&ep);
}

void test_epool_init(void) {
  int len = 5;
  struct entry_pool ep = {0};
  epool_create(&ep, len, struct test_entry, e);
  epool_init(&ep);

  TEST_ASSERT(!epool_empty(&ep));
  TEST_ASSERT_EQUAL_UINT(ep.nr_allocated, 0);
  TEST_ASSERT_EQUAL(ep.free.next, &epool_at(&ep, ep.nr_entries - 1)->pool_list);

  epool_exit(&ep);
}

void test_alloc_entry(void) {
  int len = 5;
  struct entry_pool ep = {0};
  epool_create(&ep, len, struct test_entry, e);
  epool_init(&ep);

  for (int i = 0; i < ep.nr_entries; ++i) {
    struct entry *e;

    TEST_ASSERT_EQUAL(ep.free.next,
                      &epool_at(&ep, ep.nr_entries - 1 - i)->pool_list);
    e = alloc_entry(&ep);
    TEST_ASSERT_EQUAL_UINT(ep.nr_allocated, i + 1);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQUAL(e, epool_at(&ep, ep.nr_entries - 1 - i));
    TEST_ASSERT(e->allocated);
  }

  TEST_ASSERT(epool_empty(&ep));
  epool_exit(&ep);
}

void test_alloc_particular_entry(void) {
  int len = 5;
  struct entry_pool ep = {0};
  struct entry *e;
  epool_create(&ep, len, struct test_entry, e);
  epool_init(&ep);

  e = alloc_particular_entry(&ep, len / 2);
  TEST_ASSERT_EQUAL_UINT(ep.nr_allocated, 1);
  TEST_ASSERT_NOT_NULL(e);
  TEST_ASSERT_EQUAL(e, epool_at(&ep, len / 2));
  TEST_ASSERT(e->allocated);

  epool_exit(&ep);
}

void test_epool_find(void) {
  int len = 5;
  struct entry_pool ep = {0};
  struct entry *e;
  epool_create(&ep, len, struct test_entry, e);
  epool_init(&ep);

  TEST_ASSERT_NULL(epool_find(&ep, len / 2));
  e = alloc_particular_entry(&ep, len / 2);
  TEST_ASSERT_NOT_NULL(epool_find(&ep, len / 2));

  epool_exit(&ep);
}

void test_in_pool(void) {
  int len = 5;
  struct entry_pool ep = {0};
  epool_create(&ep, len, struct test_entry, e);

  for (int i = 0; i < ep.nr_entries; ++i) {
    TEST_ASSERT(in_pool(&ep, epool_at(&ep, i)));
  }

  TEST_ASSERT_FALSE(in_pool(&ep, &len));
  TEST_ASSERT_FALSE(in_pool(&ep, &ep.entries[ep.nr_entries]));

  epool_exit(&ep);
}

void test_infer_cblock(void) {
  int len = 5;
  struct entry_pool ep = {0};
  epool_create(&ep, len, struct test_entry, e);

  for (int i = 0; i < ep.nr_entries; ++i) {
    TEST_ASSERT_EQUAL_UINT(i, infer_cblock(&ep, epool_at(&ep, i)));
  }

  epool_exit(&ep);
}

void test_free_entry(void) {
  int len = 5;
  struct entry_pool ep = {0};
  struct entry *e;
  epool_create(&ep, len, struct test_entry, e);
  epool_init(&ep);

  e = alloc_entry(&ep);
  TEST_ASSERT_EQUAL_UINT(1, ep.nr_allocated);
  free_entry(&ep, e);
  TEST_ASSERT_EQUAL_UINT(0, ep.nr_allocated);
  TEST_ASSERT_FALSE(e->allocated);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_epool_create);
  RUN_TEST(test_epool_at);
  RUN_TEST(test_epool_init);
  RUN_TEST(test_alloc_entry);
  RUN_TEST(test_alloc_particular_entry);
  RUN_TEST(test_epool_find);
  RUN_TEST(test_in_pool);
  RUN_TEST(test_infer_cblock);
  RUN_TEST(test_free_entry);
  return UNITY_END();
}
