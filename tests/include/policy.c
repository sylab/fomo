#include "include/policy.h"
#include "unity/unity.h"

struct test_entry {
  struct entry e;
};

void test_policy_init(void) {
  int size = 1;
  struct base_policy bp;
  epool_create(&bp.cache_pool, size, struct test_entry, e);
  epool_create(&bp.meta_pool, size, struct test_entry, e);
  TEST_ASSERT_EQUAL_INT(0, policy_init(&bp, size));
  TEST_ASSERT_EQUAL_UINT(size, bp.cache_size);

  epool_exit(&bp.cache_pool);
  epool_exit(&bp.meta_pool);
  policy_exit(&bp);
}

int test_map(struct base_policy *bp, oblock_t oblock, bool write,
             struct base_policy_result *result) {}
void test_remove(struct base_policy *bp, struct entry *e) {}
void test_destroy(struct base_policy *bp) {}

void test_policy_init_func(void) {
  int size = 1;
  struct base_policy bp;
  epool_create(&bp.cache_pool, size, struct test_entry, e);
  epool_create(&bp.meta_pool, size, struct test_entry, e);
  TEST_ASSERT_EQUAL_INT(0, policy_init(&bp, size));

  TEST_ASSERT_NOT_EQUAL(test_map, bp.map);
  TEST_ASSERT_NOT_EQUAL(test_remove, bp.remove);
  TEST_ASSERT_NOT_EQUAL(test_destroy, bp.destroy);
  policy_init_func(&bp, test_map, test_remove, test_destroy);
  TEST_ASSERT_EQUAL(test_map, bp.map);
  TEST_ASSERT_EQUAL(test_remove, bp.remove);
  TEST_ASSERT_EQUAL(test_destroy, bp.destroy);

  epool_exit(&bp.cache_pool);
  epool_exit(&bp.meta_pool);
  policy_exit(&bp);
}

void test_cache_size(void) {
  int size = 1;
  struct base_policy bp;
  epool_create(&bp.cache_pool, size, struct test_entry, e);
  epool_create(&bp.meta_pool, size, struct test_entry, e);
  TEST_ASSERT_EQUAL_INT(0, policy_init(&bp, size));
  TEST_ASSERT_EQUAL_UINT(size, cache_size(&bp));

  epool_exit(&bp.cache_pool);
  epool_exit(&bp.meta_pool);
  policy_exit(&bp);
}

void test_insert_in_cache_and_in_cache_and_cache_is_full(void) {
  int size = 1;
  int blkno = size + 10;
  struct entry *e;
  struct base_policy bp;
  struct base_policy_result result;
  epool_create(&bp.cache_pool, size, struct test_entry, e);
  epool_create(&bp.meta_pool, size, struct test_entry, e);
  TEST_ASSERT_EQUAL_INT(0, policy_init(&bp, size));

  TEST_ASSERT(!cache_is_full(&bp));
  TEST_ASSERT_NULL(hash_lookup(&bp.ht, blkno));
  e = insert_in_cache(&bp, blkno, &result);
  TEST_ASSERT_NOT_NULL(e);
  TEST_ASSERT_TRUE(in_cache(&bp, e));
  TEST_ASSERT_FALSE(in_meta(&bp, e));
  TEST_ASSERT(cache_is_full(&bp));
  TEST_ASSERT_EQUAL_UINT(blkno, e->oblock);
  TEST_ASSERT_EQUAL_UINT(infer_cblock(&bp.cache_pool, e), result.cblock);
  TEST_ASSERT_FALSE(e->dirty);
  TEST_ASSERT_EQUAL_UINT(1, e->hit_count);
  TEST_ASSERT_EQUAL(e, hash_lookup(&bp.ht, blkno));

  epool_exit(&bp.cache_pool);
  epool_exit(&bp.meta_pool);
  policy_exit(&bp);
}

void test_insert_in_meta_and_in_meta_and_meta_is_full(void) {
  int size = 1;
  int blkno = size + 10;
  struct entry *e;
  struct base_policy bp;
  epool_create(&bp.cache_pool, size, struct test_entry, e);
  epool_create(&bp.meta_pool, size, struct test_entry, e);
  TEST_ASSERT_EQUAL_INT(0, policy_init(&bp, size));

  TEST_ASSERT(!meta_is_full(&bp));
  TEST_ASSERT_NULL(hash_lookup(&bp.ht, blkno));
  e = insert_in_meta(&bp, blkno);
  TEST_ASSERT_NOT_NULL(e);
  TEST_ASSERT_FALSE(in_cache(&bp, e));
  TEST_ASSERT_TRUE(in_meta(&bp, e));
  TEST_ASSERT(meta_is_full(&bp));
  TEST_ASSERT_EQUAL_UINT(blkno, e->oblock);
  TEST_ASSERT_FALSE(e->dirty);
  TEST_ASSERT_EQUAL_UINT(1, e->hit_count);
  TEST_ASSERT_EQUAL(e, hash_lookup(&bp.ht, blkno));

  epool_exit(&bp.cache_pool);
  epool_exit(&bp.meta_pool);
  policy_exit(&bp);
}

void test_policy_remove(void) {
  int size = 1;
  int blkno = size + 10;
  struct entry *e;
  struct base_policy bp;
  struct base_policy_result result;
  epool_create(&bp.cache_pool, size, struct test_entry, e);
  epool_create(&bp.meta_pool, size, struct test_entry, e);
  TEST_ASSERT_EQUAL_INT(0, policy_init(&bp, size));

  e = insert_in_cache(&bp, blkno, &result);

  TEST_ASSERT_NOT_NULL(e);
  TEST_ASSERT_TRUE(in_cache(&bp, e));
  TEST_ASSERT_EQUAL(e, hash_lookup(&bp.ht, blkno));
  TEST_ASSERT_TRUE(e->allocated);
  policy_remove(&bp, e);
  TEST_ASSERT_FALSE(e->allocated);
  TEST_ASSERT_NULL(hash_lookup(&bp.ht, blkno));

  epool_exit(&bp.cache_pool);
  epool_exit(&bp.meta_pool);
  policy_exit(&bp);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_policy_init);
  RUN_TEST(test_policy_init_func);
  RUN_TEST(test_cache_size);
  RUN_TEST(test_insert_in_cache_and_in_cache_and_cache_is_full);
  RUN_TEST(test_insert_in_meta_and_in_meta_and_meta_is_full);
  RUN_TEST(test_policy_remove);
  return UNITY_END();
}
