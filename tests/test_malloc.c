#include "../include/mymalloc.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) printf("  %-40s", name)
#define PASS()                                                                 \
  do {                                                                         \
    printf("✅\n");                                                            \
    tests_passed++;                                                            \
  } while (0)
#define FAIL(msg)                                                              \
  do {                                                                         \
    printf("❌ %s\n", msg);                                                    \
    tests_failed++;                                                            \
  } while (0)

void test_basic_alloc_free() {
  TEST("Basic alloc and free");
  int *p = (int *)my_malloc(sizeof(int));
  if (p == NULL) {
    FAIL("returned NULL");
    return;
  }
  *p = 42;
  if (*p != 42) {
    FAIL("value mismatch");
    return;
  }
  my_free(p);
  PASS();
}

void test_reuse_after_free() {
  TEST("Block reuse after free");
  char *a = (char *)my_malloc(32);
  my_free(a);
  char *b = (char *)my_malloc(32);
  if (a != b) {
    FAIL("address not reused");
    return;
  }
  my_free(b);
  PASS();
}

void test_alignment() {
  TEST("8-byte alignment");
  char *p1 = (char *)my_malloc(1);
  char *p2 = (char *)my_malloc(7);
  int *p3 = (int *)my_malloc(3);
  int ok = ((uintptr_t)p1 % 8 == 0) && ((uintptr_t)p2 % 8 == 0) &&
           ((uintptr_t)p3 % 8 == 0);
  my_free(p1);
  my_free(p2);
  my_free(p3);
  if (!ok) {
    FAIL("misaligned pointer");
    return;
  }
  PASS();
}

void test_coalescing() {
  TEST("Adjacent free blocks coalesce");
  char *a = (char *)my_malloc(50);
  char *b = (char *)my_malloc(50);
  char *c = (char *)my_malloc(50);
  my_free(a);
  my_free(b);
  my_free(c);
  char *big = (char *)my_malloc(140);
  if (big != a) {
    FAIL("coalesced block not reused");
    return;
  }
  my_free(big);
  PASS();
}

void test_splitting() {
  TEST("Large block splits on small alloc");
  char *large = (char *)my_malloc(100);
  my_free(large);
  char *small = (char *)my_malloc(10);
  if (small != large) {
    FAIL("split didn't reuse base");
    return;
  }
  my_free(small);
  PASS();
}

void test_null_handling() {
  TEST("NULL and zero-size handling");
  my_free(NULL);
  void *p = my_malloc(0);
  if (p != NULL) {
    FAIL("malloc(0) should return NULL");
    return;
  }
  PASS();
}

int main() {
  printf("\n=== Custom Malloc Test Suite ===\n\n");

  test_basic_alloc_free();
  test_reuse_after_free();
  test_alignment();
  test_coalescing();
  test_splitting();
  test_null_handling();

  printf("\n--- Results: %d passed, %d failed ---\n\n", tests_passed,
         tests_failed);
  return tests_failed > 0 ? 1 : 0;
}