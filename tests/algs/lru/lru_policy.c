#include "algs/lru/lru_policy.c"
#include "unity/unity.h"

#define LRU_SIZE 2

void test_lru_create(void) {
  struct lru_policy *lru;
  struct base_policy *bp = lru_create(LRU_SIZE);
  TEST_ASSERT_NOT_NULL(bp);
  lru = to_lru_policy(bp);
  TEST_ASSERT_EQUAL(bp, &lru->policy);
  lru_destroy(bp);
}

void test_lru_init(void) {
  struct lru_policy *lru;
  struct base_policy *bp = lru_create(LRU_SIZE);
  TEST_ASSERT_NOT_NULL(bp);
  lru = to_lru_policy(bp);
  TEST_ASSERT_EQUAL_INT(0, lru_init(lru, LRU_SIZE));
  TEST_ASSERT_EQUAL_UINT(0, queue_length(&lru->lru_q));
  TEST_ASSERT_EQUAL_UINT(LRU_SIZE, bp->cache_pool.nr_entries);
  TEST_ASSERT_EQUAL_UINT(0, bp->meta_pool.nr_entries);

  lru_destroy(bp);
}

void test_lru_miss(void) {
  struct entry *e;
  struct base_policy_result result;
  struct lru_policy *lru;
  struct base_policy *bp = lru_create(LRU_SIZE);
  TEST_ASSERT_NOT_NULL(bp);
  lru = to_lru_policy(bp);

  TEST_ASSERT_EQUAL_UINT(0, bp->stats.misses);
  TEST_ASSERT_EQUAL_UINT(0, bp->stats.promotions);
  TEST_ASSERT_EQUAL_UINT(0, lru_miss(lru, 1, &result));
  e = hash_lookup(&bp->ht, 1);
  TEST_ASSERT_NOT_NULL(e);
  TEST_ASSERT_EQUAL(e, epool_find(&bp->cache_pool, result.cblock));
  TEST_ASSERT_TRUE(in_cache(bp, e));
  TEST_ASSERT_EQUAL_UINT(1, bp->stats.misses);
  TEST_ASSERT_EQUAL_UINT(1, bp->stats.promotions);
  TEST_ASSERT_EQUAL(POLICY_NEW, result.op);

  lru_destroy(bp);
}

void test_lru_hit(void) {
  struct entry *e;
  struct base_policy_result result;
  struct lru_policy *lru;
  struct base_policy *bp = lru_create(LRU_SIZE);
  TEST_ASSERT_NOT_NULL(bp);
  lru = to_lru_policy(bp);

  TEST_ASSERT_EQUAL_UINT(0, bp->stats.hits);
  TEST_ASSERT_EQUAL_UINT(0, lru_miss(lru, 1, &result));
  e = hash_lookup(&bp->ht, 1);
  TEST_ASSERT_NOT_NULL(e);
  TEST_ASSERT_EQUAL_UINT(0, lru_hit(lru, e, &result));
  TEST_ASSERT_EQUAL_UINT(1, bp->stats.hits);
  TEST_ASSERT_EQUAL(POLICY_HIT, result.op);

  lru_destroy(bp);
}

void test_lru_evict(void) {
  struct entry *e;
  struct base_policy_result result;
  struct lru_policy *lru;
  struct base_policy *bp = lru_create(LRU_SIZE);
  TEST_ASSERT_NOT_NULL(bp);
  lru = to_lru_policy(bp);

  TEST_ASSERT_EQUAL_UINT(0, bp->stats.demotions);
  TEST_ASSERT_EQUAL_UINT(0, lru_miss(lru, 1, &result));
  lru_evict(lru, &result.old_oblock);
  TEST_ASSERT_EQUAL_UINT(1, result.old_oblock);
  TEST_ASSERT_EQUAL_UINT(1, bp->stats.demotions);

  lru_destroy(bp);
}

void test_lru_map_hit_and_miss(void) {
  struct entry *e;
  struct base_policy_result result;
  struct base_policy *bp = lru_create(LRU_SIZE);
  TEST_ASSERT_NOT_NULL(bp);

  TEST_ASSERT_EQUAL_UINT(0, lru_map(bp, 1, false, &result));
  TEST_ASSERT_EQUAL(POLICY_NEW, result.op);
  TEST_ASSERT_EQUAL_UINT(0, bp->stats.hits);
  TEST_ASSERT_EQUAL_UINT(1, bp->stats.misses);
  TEST_ASSERT_EQUAL_UINT(0, lru_map(bp, 2, false, &result));
  TEST_ASSERT_EQUAL(POLICY_NEW, result.op);
  TEST_ASSERT_EQUAL_UINT(0, bp->stats.hits);
  TEST_ASSERT_EQUAL_UINT(2, bp->stats.misses);
  TEST_ASSERT_EQUAL_UINT(0, lru_map(bp, 2, false, &result));
  TEST_ASSERT_EQUAL(POLICY_HIT, result.op);
  TEST_ASSERT_EQUAL_UINT(1, bp->stats.hits);
  TEST_ASSERT_EQUAL_UINT(2, bp->stats.misses);

  lru_destroy(bp);
}

void test_lru_full(void) {
  struct entry *e;
  struct base_policy_result result;
  struct base_policy *bp = lru_create(LRU_SIZE);
  TEST_ASSERT_NOT_NULL(bp);

  TEST_ASSERT_EQUAL_UINT(0, lru_map(bp, 1, false, &result));
  TEST_ASSERT_EQUAL_UINT(0, lru_map(bp, 2, false, &result));
  TEST_ASSERT_EQUAL_UINT(0, lru_map(bp, 3, false, &result));
  TEST_ASSERT_EQUAL(POLICY_REPLACE, result.op);
  TEST_ASSERT_EQUAL_UINT(1, result.old_oblock);
  TEST_ASSERT_EQUAL_UINT(3, bp->stats.misses);
  TEST_ASSERT_EQUAL_UINT(3, bp->stats.promotions);
  TEST_ASSERT_EQUAL_UINT(1, bp->stats.demotions);

  lru_destroy(bp);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_lru_create);
  RUN_TEST(test_lru_init);
  RUN_TEST(test_lru_miss);
  RUN_TEST(test_lru_hit);
  RUN_TEST(test_lru_evict);
  RUN_TEST(test_lru_map_hit_and_miss);
  RUN_TEST(test_lru_full);
  return UNITY_END();
}
