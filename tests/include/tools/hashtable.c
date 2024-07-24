#include "../src/include/tools/hashtable.h"
#include "unity/unity.h"

void test_next_power(void) {
  TEST_ASSERT_EQUAL_UINT(32, next_power(17, 16));
  TEST_ASSERT_EQUAL_UINT(64, next_power(33, 16));
}

void test_hash_init(void) {
  int size = 5;
  struct hashtable ht;
  hash_init(&ht, size);
  TEST_ASSERT_EQUAL_UINT(16, ht.nr_buckets);

  hash_exit(&ht);
}

void test_hash_insert_and_lookup(void) {
  int size = 5;
  struct hashtable ht;
  struct entry e;
  e.oblock = size;
  hash_init(&ht, size);
  TEST_ASSERT_NULL(hash_lookup(&ht, size));
  hash_insert(&ht, &e);
  TEST_ASSERT_NOT_NULL(hash_lookup(&ht, size));

  hash_exit(&ht);
}

void test_hash_remove(void) {
  int size = 5;
  struct hashtable ht;
  struct entry e;
  e.oblock = size;
  hash_init(&ht, size);
  hash_insert(&ht, &e);
  TEST_ASSERT_NOT_NULL(hash_lookup(&ht, size));
  hash_remove(&e);
  TEST_ASSERT_NULL(hash_lookup(&ht, size));

  hash_exit(&ht);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_next_power);
  RUN_TEST(test_hash_init);
  RUN_TEST(test_hash_insert_and_lookup);
  RUN_TEST(test_hash_remove);
  return UNITY_END();
}
