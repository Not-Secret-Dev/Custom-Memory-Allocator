#define _GNU_SOURCE
#include "../include/mymalloc.h"
#include "internal.h"
#include <string.h>
#include <unistd.h>

// Definition of global state
void *heap_start = NULL;

static block_header_t *find_free_block(size_t size) {
  block_header_t *current = (block_header_t *)heap_start;
  while (current != NULL) {
    if (current->free == 1 && current->size >= size + sizeof(block_header_t)) {
      return current;
    }
    current = current->next;
  }
  return NULL;
}

static void split_block(block_header_t *block, size_t size) {
  size_t remainder = block->size - size - sizeof(block_header_t);
  if (remainder >= MIN_BLOCK_SIZE) {
    block_header_t *new_block =
        (block_header_t *)((char *)block + sizeof(block_header_t) + size);
    new_block->size = remainder;
    new_block->free = 1;
    new_block->next = block->next;

    block->size = size + sizeof(block_header_t);
    block->next = new_block;
  }
}

static block_header_t *extend_heap(size_t total_size) {
  void *new_memory = sbrk(total_size);
  if (new_memory == (void *)-1) {
    return NULL;
  }

  block_header_t *new_block = (block_header_t *)new_memory;
  new_block->size = total_size;
  new_block->free = 0;
  new_block->next = NULL;

  // Append to end of list
  block_header_t *last = (block_header_t *)heap_start;
  while (last->next != NULL) {
    last = last->next;
  }
  last->next = new_block;

  return new_block;
}

static void coalesce(block_header_t *header) {
  // Backward coalescing
  block_header_t *prev = NULL;
  block_header_t *curr = (block_header_t *)heap_start;
  while (curr != NULL && curr != header) {
    prev = curr;
    curr = curr->next;
  }
  if (prev != NULL && prev->free == 1) {
    prev->size += header->size;
    prev->next = header->next;
    header = prev;
  }

  // Forward coalescing
  if (header->next != NULL && header->next->free == 1) {
    header->size += header->next->size;
    header->next = header->next->next;
  }
}

// === PUBLIC API ===

void *my_malloc(size_t size) {
  if (size == 0)
    return NULL;

  size = ALIGN(size);

  // First-time initialization
  if (heap_start == NULL) {
    void *request = sbrk(INITIAL_HEAP_SIZE);
    if (request == (void *)-1)
      return NULL;

    block_header_t *first = (block_header_t *)request;
    first->size = INITIAL_HEAP_SIZE;
    first->free = 1;
    first->next = NULL;
    heap_start = first;
  }

  // Search for free block
  block_header_t *block = find_free_block(size);
  if (block != NULL) {
    split_block(block, size);
    block->free = 0;
    return (void *)((char *)block + sizeof(block_header_t));
  }

  // Extend heap
  size_t total_size = size + sizeof(block_header_t);
  block_header_t *new_block = extend_heap(total_size);
  if (new_block == NULL)
    return NULL;

  return (void *)((char *)new_block + sizeof(block_header_t));
}

void my_free(void *ptr) {
  if (ptr == NULL)
    return;

  block_header_t *header =
      (block_header_t *)((char *)ptr - sizeof(block_header_t));
  header->free = 1;
  coalesce(header);
}