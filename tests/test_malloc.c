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

void test_realloc_null() {
  TEST("realloc(NULL, n) acts as malloc");
  int *p = (int *)my_realloc(NULL, sizeof(int));
  if (p == NULL) {
    FAIL("returned NULL");
    return;
  }
  *p = 99;
  if (*p != 99) {
    FAIL("value mismatch");
    return;
  }
  my_free(p);
  PASS();
}

void test_realloc_zero() {
  TEST("realloc(ptr, 0) acts as free");
  char *p = (char *)my_malloc(32);
  void *result = my_realloc(p, 0);
  if (result != NULL) {
    FAIL("should return NULL");
    return;
  }
  PASS();
}

void test_realloc_grow_in_place() {
  TEST("realloc grows without moving when possible");
  char *p = (char *)my_malloc(16);
  char *original = p;
  p = (char *)my_realloc(p, 32);
  if (p != original) {
    FAIL("pointer moved unnecessarily");
    return;
  }
  my_free(p);
  PASS();
}

void test_realloc_shrink() {
  TEST("realloc shrinks correctly");
  char *p = (char *)my_malloc(100);
  p = (char *)my_realloc(p, 16);
  if (p == NULL) {
    FAIL("returned NULL");
    return;
  }
  my_free(p);
  PASS();
}

void test_realloc_preserves_data() {
  TEST("realloc preserves data on move");
  char *p = (char *)my_malloc(8);
  strcpy(p, "HELLO");
  // Force a move by allocating something between p and next free space
  char *blocker = (char *)my_malloc(8);
  p = (char *)my_realloc(p, 256);
  if (strcmp(p, "HELLO") != 0) {
    FAIL("data corrupted");
    my_free(p);
    my_free(blocker);
    return;
  }
  my_free(p);
  my_free(blocker);
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
  test_realloc_null();
  test_realloc_grow_in_place();
  test_realloc_preserves_data();
  test_realloc_shrink();
  test_realloc_zero();

  printf("\n--- Results: %d passed, %d failed ---\n\n", tests_passed,
         tests_failed);
  return tests_failed > 0 ? 1 : 0;
}