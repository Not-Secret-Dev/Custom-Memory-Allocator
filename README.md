# Custom Malloc

A lightweight custom memory allocator written in **C**, implementing simplified versions of `malloc()`, `free()`, and `realloc()` using the Unix `sbrk()` system call.

The allocator manages its own heap, tracks memory blocks using metadata, reuses freed memory, splits oversized blocks, coalesces adjacent free blocks, and can resize existing allocations both in-place and by moving them when necessary.

> **Project status:** Functional educational allocator demonstrating fundamental dynamic memory-management and systems-programming concepts.

---

## Features

* Custom `my_malloc()` implementation
* Custom `my_free()` implementation
* Custom `my_realloc()` implementation
* Dynamic heap expansion using `sbrk()`
* 8-byte memory alignment
* First-fit free-block searching
* Freed-block reuse
* Block splitting
* Adjacent-block coalescing
* In-place `realloc()` expansion when possible
* `realloc()` fallback with memory copying
* Shrinking allocations with `realloc()`
* Doubly linked block list
* `NULL` and zero-size handling
* Automated test suite
* Minimal public API with allocator internals separated from the interface

---

## How It Works

The allocator maintains a doubly linked list of memory blocks.

Each block contains a header storing metadata about the allocation:

```text
                    block_header_t
              ┌──────────────────────┐
              │ size                 │
              │ free                 │
              │ prev ────────────────┼──────► previous block
              │ next ────────────────┼──────► next block
              └──────────────────────┘
                         │
                         ▼
              ┌──────────────────────┐
              │                      │
              │    User Memory       │
              │                      │
              └──────────────────────┘
```

The allocator's public API exposes three operations:

```c
void *my_malloc(size_t size);
void my_free(void *ptr);
void *my_realloc(void *ptr, size_t size);
```

---

# Allocation

When `my_malloc()` is called, the allocator:

1. Rejects a zero-size allocation.
2. Aligns the requested size to an 8-byte boundary.
3. Initializes the custom heap on the first allocation.
4. Searches the block list for a sufficiently large free block.
5. Reuses the first suitable free block.
6. Splits the block if enough space remains for another block.
7. Extends the process heap with `sbrk()` if no suitable block exists.
8. Returns a pointer to the usable memory after the block header.

### First-Fit Strategy

The allocator uses a **first-fit** allocation strategy.

Instead of searching for the smallest possible free block, it walks the list and selects the first block large enough to satisfy the request.

```text
Heap:

┌──────────┐  ┌──────────┐  ┌──────────────┐
│ Allocated│  │   Free   │  │     Free     │
│          │  │  64 bytes│  │   256 bytes  │
└──────────┘  └──────────┘  └──────────────┘
                   ▲
                   │
             first suitable block
```

This keeps the allocation algorithm simple while demonstrating a common memory-allocation strategy.

---

# Freeing Memory

When `my_free()` is called:

1. The allocator locates the block header belonging to the supplied pointer.
2. Marks the block as free.
3. Updates the heap tail when necessary.
4. Attempts to coalesce the block with neighboring free blocks.

For example:

```text
Before:

┌──────────┬──────────┬──────────┐
│ Allocated│   Free   │ Allocated│
└──────────┴──────────┴──────────┘

          my_free()

┌──────────┬──────────┬──────────┐
│ Allocated│   Free   │ Allocated│
└──────────┴──────────┴──────────┘
```

If neighboring blocks are also free, they can be merged into a larger block.

---

# Block Splitting

When a free block is larger than the requested allocation, the allocator can split it.

```text
Before:

┌─────────────────────────────────────────────┐
│                Free Block                   │
└─────────────────────────────────────────────┘


After:

┌───────────────────┬─────────────────────────┐
│    Allocated      │          Free           │
│      Block        │          Block          │
└───────────────────┴─────────────────────────┘
```

Splitting allows the unused remainder to be reused by later allocations.

The allocator only performs the split when the remaining space is large enough to hold another block header and usable memory.

---

# Block Coalescing

Repeated allocation and freeing can fragment memory.

The allocator therefore attempts to combine adjacent free blocks.

```text
Before:

┌──────────┬──────────┬──────────┐
│   Free   │   Free   │   Free   │
└──────────┴──────────┴──────────┘


After:

┌────────────────────────────────┐
│        Large Free Block        │
└────────────────────────────────┘
```

Coalescing increases the size of available free regions and can allow larger future allocations to reuse previously fragmented memory.

---

# Reallocation

`my_realloc()` allows an existing allocation to change size while preserving its existing data.

```c
void *my_realloc(void *ptr, size_t size);
```

The implementation handles several cases.

### `ptr == NULL`

A `NULL` pointer behaves like a normal allocation:

```c
my_realloc(NULL, size);
```

is equivalent to:

```c
my_malloc(size);
```

---

### `size == 0`

A zero-size reallocation releases the existing block:

```c
my_realloc(ptr, 0);
```

The function frees the allocation and returns `NULL`.

---

### Shrinking an Allocation

If the requested size is smaller than the existing allocation, the allocator keeps the same pointer.

When enough space remains, the block is split so the unused portion can become a separate free block.

```text
Before:

┌──────────────────────────────────────┐
│            128-byte block            │
└──────────────────────────────────────┘


realloc(ptr, 32)


After:

┌────────────────┬─────────────────────┐
│   32 bytes     │      Free space     │
└────────────────┴─────────────────────┘
```

---

### Expanding In-Place

When the block immediately following the allocation is free and contains enough space, `my_realloc()` can expand the existing allocation without moving the data.

```text
Before:

┌──────────────┬──────────────────────┐
│   Allocated  │        Free          │
│     Block    │        Block         │
└──────────────┴──────────────────────┘


realloc(ptr, larger_size)


After:

┌──────────────────────────────────────┐
│          Expanded Allocation         │
└──────────────────────────────────────┘
```

This avoids allocating another region and copying the existing contents.

---

### Moving an Allocation

If the adjacent block cannot provide enough space, the allocator falls back to:

1. Allocate a new block.
2. Copy the existing data with `memcpy()`.
3. Free the original block.
4. Return the new pointer.

Conceptually:

```text
Old:

┌──────────────────┐
│ Existing Data    │
└──────────────────┘
          │
          │ memcpy()
          ▼
New:

┌─────────────────────────────┐
│ Existing Data + Extra Space │
└─────────────────────────────┘

Old block → freed
```

This provides the expected resizing behavior even when an allocation cannot be expanded in place.

---

# Memory Layout

Every allocation consists of allocator metadata followed by user-accessible memory.

```text
                    Heap Memory
                        │
                        ▼

┌─────────────────────────────────────────────┐
│ Block Header                                │
│                                             │
│ size                                        │
│ free                                        │
│ prev ──────────────────────────────┐        │
│ next ────────────────────────┐     │        │
└──────────────────────────────│─────│────────┘
                               │     │
                               │     └────────► Previous block
                               │
                               └──────────────► Next block

┌─────────────────────────────────────────────┐
│                                             │
│              User Allocation                │
│                                             │
└─────────────────────────────────────────────┘
```

The pointer returned to the caller points immediately after the block header.

---

# Heap Management

The allocator obtains memory from the operating system using:

```c
sbrk();
```

The initial heap reservation is:

```c
#define INITIAL_HEAP_SIZE 4096
```

When the allocator cannot satisfy a request using existing free blocks, it increases the process heap and creates a new block.

The allocator also maintains:

* `heap_start` — first block in the heap
* `heap_tail` — final block in the heap

Each block contains both `next` and `prev` pointers, allowing the allocator to maintain a doubly linked list.

---

# Alignment

Requested sizes are rounded up to an 8-byte boundary:

```c
#define ALIGNMENT 8
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))
```

For example:

```text
Requested     Aligned

1 byte    →   8 bytes
7 bytes   →   8 bytes
9 bytes   →   16 bytes
17 bytes  →   24 bytes
```

Proper alignment is important because C data types can have specific alignment requirements.

---

# Project Structure

```text
custom malloc/
├── include/
│   └── mymalloc.h          # Public allocator API
│
├── src/
│   ├── internal.h          # Internal allocator structures/macros
│   └── mymalloc.c          # Allocator implementation
│
├── tests/
│   ├── test_malloc.c       # Automated test suite
│   └── run_tests           # Compiled test executable
│
└── Makefile                # Build and test commands
```

---

# API

## `my_malloc`

```c
void *my_malloc(size_t size);
```

Allocates `size` bytes and returns a pointer to the allocated memory.

Returns `NULL` when:

* `size == 0`
* the allocator cannot obtain additional memory

---

## `my_free`

```c
void my_free(void *ptr);
```

Releases a previously allocated block back to the custom allocator.

Passing `NULL` is safely ignored.

---

## `my_realloc`

```c
void *my_realloc(void *ptr, size_t size);
```

Changes the size of an existing allocation while preserving its contents.

The implementation supports:

* `NULL` pointer allocation
* shrinking existing allocations
* in-place expansion when possible
* moving allocations when necessary
* freeing an allocation when the requested size is zero

---

# Example

```c
#include "mymalloc.h"

int main(void)
{
    int *numbers = my_malloc(5 * sizeof(int));

    if (numbers == NULL)
        return 1;

    for (int i = 0; i < 5; i++)
        numbers[i] = i * 10;

    int *resized = my_realloc(numbers, 10 * sizeof(int));

    if (resized == NULL)
        return 1;

    numbers = resized;

    for (int i = 5; i < 10; i++)
        numbers[i] = i * 10;

    my_free(numbers);

    return 0;
}
```

---

# Building

## Requirements

* Linux/Unix-like operating system
* GCC
* GNU `sbrk()` support
* GNU Make

## Build

```bash
make
```

## Run Tests

```bash
make test
```

## Clean Build Artifacts

```bash
make clean
```

---

# Testing

The project includes an automated test suite covering the core allocator functionality.

Current tests include:

| Test                      | Purpose                                               |
| ------------------------- | ----------------------------------------------------- |
| Basic allocation and free | Verifies allocation, memory access, and deallocation  |
| Block reuse               | Verifies freed blocks can be reused                   |
| Alignment                 | Verifies returned pointers are 8-byte aligned         |
| Coalescing                | Verifies adjacent free blocks can be merged           |
| Splitting                 | Verifies large blocks can satisfy smaller allocations |
| NULL / zero-size handling | Verifies allocator edge cases                         |

Run the test suite with:

```bash
make test
```

---

# Design Decisions

### First-Fit Allocation

The allocator uses first-fit searching rather than a more complex allocation strategy.

This keeps the implementation straightforward and makes the behavior easy to reason about.

### Doubly Linked Block List

Each block stores both `next` and `prev` pointers.

```text
┌────────┐      ┌────────┐      ┌────────┐
│ Block  │◄────►│ Block  │◄────►│ Block  │
└────────┘      └────────┘      └────────┘
```

This allows the allocator to maintain relationships between neighboring blocks while tracking the beginning and end of the heap.

### Splitting

Large free blocks are split when the remaining space can form another valid allocation block.

### Coalescing

Adjacent free blocks are merged to reduce fragmentation and create larger reusable regions.

### In-Place Reallocation

`my_realloc()` first attempts to resize an allocation without moving it when the following block is free and provides enough space.

Only when that approach fails does it allocate a new block and copy the existing contents.

This avoids unnecessary data movement when possible.

---

# Limitations

This project intentionally implements a simplified allocator rather than a production-quality replacement for the system allocator.

Current limitations include:

* Not thread-safe
* No protection against double-free
* No validation of arbitrary pointers passed to `my_free()`
* No `calloc()` implementation
* Uses a linear free-block search
* No sophisticated fragmentation-management strategy
* Does not return unused memory to the operating system
* Relies on `sbrk()`, which is obsolete/deprecated for modern allocator design
* Allocator metadata is not protected against memory corruption
* No segregated free lists or size classes
* No thread-local caches
* No concurrency support

These limitations are intentional. The goal is to explore the fundamental mechanisms behind dynamic memory allocation rather than reproduce the complexity of production allocators such as `glibc`'s `malloc`.

---

# Concepts Demonstrated

This project explores several low-level C and operating-system concepts:

* Dynamic memory management
* Process heap management
* Unix system calls
* `sbrk()`
* Pointer arithmetic
* Struct layout
* Memory alignment
* Doubly linked lists
* Free lists
* First-fit allocation
* Memory fragmentation
* Block splitting
* Block coalescing
* In-place memory resizing
* `memcpy()`
* Manual resource management
* C build systems with Make
* Automated testing

---

# What I Learned

Building a memory allocator from scratch provides a practical look at what happens beneath the standard C allocation APIs.

The project demonstrates how an allocator can:

* Maintain metadata for individual memory blocks
* Track free and allocated regions
* Reuse previously released memory
* Split large blocks into smaller allocations
* Merge adjacent free blocks
* Expand the process heap
* Resize allocations without always moving them
* Move and copy allocations when in-place expansion is impossible
* Handle alignment requirements
* Manage memory using pointer arithmetic and linked data structures

Rather than treating:

```c
malloc();
free();
realloc();
```

as black-box operations, this project explores the core ideas that make those interfaces possible.

---

# Why I Built This

I built this project to strengthen my understanding of **C, operating-system memory management, pointers, data structures, and systems programming**.

Instead of only using dynamic memory allocation through the standard library, I wanted to understand how an allocator could manage its own heap and implement the fundamental behaviors behind allocation, deallocation, and resizing.

The project is intentionally small enough to understand end-to-end while still exposing real systems-programming concepts such as heap growth, metadata management, fragmentation, block reuse, and memory movement.

---

## Disclaimer

**This is an educational implementation and should not be used as a replacement for the system allocator in production software.**

It is designed for learning, experimentation, and exploring the fundamentals of dynamic memory management.
