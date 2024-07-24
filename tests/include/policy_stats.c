#include "include/policy_stats.h"
#include "unity/unity.h"

void test_stats_init(void) {
  struct policy_stats stats;
  stats_init(&stats);
  TEST_ASSERT_EQUAL_UINT(0, stats.hits);
  TEST_ASSERT_EQUAL_UINT(0, stats.misses);
  TEST_ASSERT_EQUAL_UINT(0, stats.filters);
  TEST_ASSERT_EQUAL_UINT(0, stats.promotions);
  TEST_ASSERT_EQUAL_UINT(0, stats.demotions);
}

void test_inc_hits(void) {
  struct policy_stats stats;
  stats_init(&stats);
  inc_hits(&stats);
  TEST_ASSERT_EQUAL_UINT(1, stats.hits);
  TEST_ASSERT_EQUAL_UINT(0, stats.misses);
  TEST_ASSERT_EQUAL_UINT(0, stats.filters);
  TEST_ASSERT_EQUAL_UINT(0, stats.promotions);
  TEST_ASSERT_EQUAL_UINT(0, stats.demotions);
}

void test_inc_misses(void) {
  struct policy_stats stats;
  stats_init(&stats);
  inc_misses(&stats);
  TEST_ASSERT_EQUAL_UINT(0, stats.hits);
  TEST_ASSERT_EQUAL_UINT(1, stats.misses);
  TEST_ASSERT_EQUAL_UINT(0, stats.filters);
  TEST_ASSERT_EQUAL_UINT(0, stats.promotions);
  TEST_ASSERT_EQUAL_UINT(0, stats.demotions);
}

void test_inc_filters(void) {
  struct policy_stats stats;
  stats_init(&stats);
  inc_filters(&stats);
  TEST_ASSERT_EQUAL_UINT(0, stats.hits);
  TEST_ASSERT_EQUAL_UINT(0, stats.misses);
  TEST_ASSERT_EQUAL_UINT(1, stats.filters);
  TEST_ASSERT_EQUAL_UINT(0, stats.promotions);
  TEST_ASSERT_EQUAL_UINT(0, stats.demotions);
}

void test_inc_promotions(void) {
  struct policy_stats stats;
  stats_init(&stats);
  inc_promotions(&stats);
  TEST_ASSERT_EQUAL_UINT(0, stats.hits);
  TEST_ASSERT_EQUAL_UINT(0, stats.misses);
  TEST_ASSERT_EQUAL_UINT(0, stats.filters);
  TEST_ASSERT_EQUAL_UINT(1, stats.promotions);
  TEST_ASSERT_EQUAL_UINT(0, stats.demotions);
}

void test_inc_demotions(void) {
  struct policy_stats stats;
  stats_init(&stats);
  inc_demotions(&stats);
  TEST_ASSERT_EQUAL_UINT(0, stats.hits);
  TEST_ASSERT_EQUAL_UINT(0, stats.misses);
  TEST_ASSERT_EQUAL_UINT(0, stats.filters);
  TEST_ASSERT_EQUAL_UINT(0, stats.promotions);
  TEST_ASSERT_EQUAL_UINT(1, stats.demotions);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_stats_init);
  RUN_TEST(test_inc_hits);
  RUN_TEST(test_inc_misses);
  RUN_TEST(test_inc_filters);
  RUN_TEST(test_inc_promotions);
  RUN_TEST(test_inc_demotions);
  return UNITY_END();
}
