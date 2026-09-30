#include <stddef.h>
#define _GNU_SOURCE
#include "../include/mymalloc.h"
#include "internal.h"
#include <string.h>
#include <unistd.h>

// Definition of global state
void *heap_start = NULL;
void *heap_tail = NULL;

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
    new_block->prev = block->prev;

    if (block == heap_tail) {
      heap_tail = new_block;
    } else if (block->next != NULL) {
      block->next->prev = new_block;
    }

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
  new_block->prev = (block_header_t *)heap_tail;
  if (heap_start == NULL) {
    heap_start = new_block;
  } else {
    ((block_header_t *)heap_tail)->next = new_block;
  }
  heap_tail = new_block;

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
    if (header->next == heap_tail) {
      heap_tail = header;
    }
    header->size += header->next->size;
    header->next = header->next->next;
    if (header->next != NULL) {
      header->next->prev = header;
    }
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
    heap_tail = first;
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

void *my_realloc(void *ptr, size_t size) {
  if (ptr == NULL) {
    return my_malloc(size);
  }

  if (size == 0) {
    my_free(ptr);
    return NULL;
  }

  size = ALIGN(size);
  block_header_t *header =
      (block_header_t *)((char *)ptr - sizeof(block_header_t));
  size_t old_usable_size = header->size - sizeof(block_header_t);
  if (size <= old_usable_size) {
    size_t remainder = old_usable_size - size;
    if (remainder >= MIN_BLOCK_SIZE) {
      split_block(header, size);
      block_header_t *remainder_block = header->next;
      if (remainder_block != NULL && remainder_block->free == 1) {
        coalesce(remainder_block);
      }
    }
    return ptr;
  }

  if (header->next != NULL && header->next->free == 1) {
    size_t combined_usable = old_usable_size + header->next->size;
    if (combined_usable >= size) {
      header->size = header->next->size;
      header->next = header->next->next;

      size_t new_remainder = header->size - sizeof(block_header_t) - size;
      if (new_remainder >= MIN_BLOCK_SIZE) {
        split_block(header, size);
      }
      return ptr;
    }
  }

  void *new_ptr = my_malloc(size);
  if (new_ptr == NULL) {
    return NULL;
  }

  memcpy(new_ptr, ptr, old_usable_size);
  my_free(ptr);
  return new_ptr;
}

void my_free(void *ptr) {
  if (ptr == NULL)
    return;

  block_header_t *header =
      (block_header_t *)((char *)ptr - sizeof(block_header_t));
  header->free = 1;

  if (header == heap_tail) {
    heap_tail = header->prev;
  }
  coalesce(header);
}