#include "sim/include/registry.h"
#include "unity/unity.h"

struct registry_entry test_entry1 = {
    .name = "test_entry1",
};

struct registry_entry test_entry1_copy = {
    .name = "test_entry1",
};

struct registry_entry test_entry2 = {
    .name = "test_entry2",
};

void test_registry_init(void) {
  struct registry r;
  registry_init(&r);
  TEST_ASSERT_TRUE(list_empty(&r.list));
}

void test_registry_find_nothing(void) {
  struct registry r;
  registry_init(&r);
  TEST_ASSERT_NULL(registry_find(&r, test_entry1.name));
}

void test_registry_push(void) {
  struct registry r;
  registry_init(&r);
  registry_push(&r, &test_entry1);
  TEST_ASSERT_FALSE(list_empty(&r.list));
}

void test_registry_find_pushed(void) {
  struct registry_entry *re;
  struct registry r;
  registry_init(&r);
  registry_push(&r, &test_entry1);
  registry_push(&r, &test_entry2);

  re = registry_find(&r, test_entry1.name);
  TEST_ASSERT_NOT_NULL(re);
  TEST_ASSERT_EQUAL(&test_entry1, re);

  re = registry_find(&r, test_entry2.name);
  TEST_ASSERT_NOT_NULL(re);
  TEST_ASSERT_EQUAL(&test_entry2, re);
}

void test_registry_remove(void) {
  struct registry r;
  registry_init(&r);
  registry_push(&r, &test_entry1);
  registry_push(&r, &test_entry2);

  TEST_ASSERT_FALSE(list_empty(&r.list));
  registry_remove(&r, test_entry1.name);
  TEST_ASSERT_FALSE(list_empty(&r.list));
  registry_remove(&r, test_entry2.name);
  TEST_ASSERT_TRUE(list_empty(&r.list));
}

void test_registry_cant_double_push_or_remove(void) {
  struct registry r;
  registry_init(&r);
  registry_push(&r, &test_entry1);
  registry_push(&r, &test_entry1_copy); // silently not added
  TEST_ASSERT_EQUAL(&test_entry1, registry_find(&r, test_entry1_copy.name));

  registry_remove(&r, test_entry1.name);
  TEST_ASSERT_NULL(registry_find(&r, test_entry1_copy.name));
  registry_remove(&r, test_entry1_copy.name);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_registry_init);
  RUN_TEST(test_registry_find_nothing);
  RUN_TEST(test_registry_push);
  RUN_TEST(test_registry_find_pushed);
  RUN_TEST(test_registry_remove);
  RUN_TEST(test_registry_cant_double_push_or_remove);
  return UNITY_END();
}
