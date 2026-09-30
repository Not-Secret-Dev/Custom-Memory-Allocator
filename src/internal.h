#ifndef INTERNAL_H
#define INTERNAL_H

#include <stddef.h>
#include <stdint.h>

#define ALIGNMENT 8
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))
#define MIN_BLOCK_SIZE (sizeof(block_header_t) + 8)
#define INITIAL_HEAP_SIZE 4096

typedef struct block_header {
  size_t size;
  int free;
  struct block_header *next;
  struct block_header *prev;
} block_header_t;

extern void *heap_start;
extern void *heap_tail;

#endif