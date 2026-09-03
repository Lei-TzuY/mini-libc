#include <errno.h>
#include <mini/syscall.h>
#include <stddef.h>
#include <stdlib.h>

#define MINI_ALLOC_ALIGNMENT 16UL

struct mini_block {
    size_t size;
    struct mini_block *next;
    unsigned long is_free;
    unsigned long reserved;
};

_Static_assert(sizeof(struct mini_block) == 32,
               "allocator header must preserve 16-byte payload alignment");

static struct mini_block *block_head;
static struct mini_block *block_tail;
static __UINTPTR_TYPE__ heap_end;
static int heap_initialized;

static int align_size(size_t size, size_t *aligned)
{
    size_t max = (size_t)-1;

    if (size > max - (MINI_ALLOC_ALIGNMENT - 1UL)) {
        return 0;
    }
    *aligned = (size + (MINI_ALLOC_ALIGNMENT - 1UL)) &
               ~(size_t)(MINI_ALLOC_ALIGNMENT - 1UL);
    return 1;
}

static int initialize_heap(void)
{
    __UINTPTR_TYPE__ current;
    __UINTPTR_TYPE__ aligned;
    long result;

    if (heap_initialized) {
        return 1;
    }

    result = mini_sys_brk((void *)0);
    if (result <= 0) {
        return 0;
    }
    current = (__UINTPTR_TYPE__)result;
    if (current > (__UINTPTR_TYPE__)-1 - (MINI_ALLOC_ALIGNMENT - 1UL)) {
        return 0;
    }
    aligned = (current + (MINI_ALLOC_ALIGNMENT - 1UL)) &
              ~(__UINTPTR_TYPE__)(MINI_ALLOC_ALIGNMENT - 1UL);

    if (aligned != current) {
        result = mini_sys_brk((void *)aligned);
        if ((__UINTPTR_TYPE__)result != aligned) {
            return 0;
        }
    }

    heap_end = aligned;
    heap_initialized = 1;
    return 1;
}

static void split_block(struct mini_block *block, size_t size)
{
    size_t remaining = block->size - size;

    if (remaining < sizeof(struct mini_block) + MINI_ALLOC_ALIGNMENT) {
        return;
    }

    {
        struct mini_block *split =
            (struct mini_block *)((unsigned char *)(block + 1) + size);

        split->size = remaining - sizeof(struct mini_block);
        split->next = block->next;
        split->is_free = 1;
        split->reserved = 0;
        block->size = size;
        block->next = split;
        if (block_tail == block) {
            block_tail = split;
        }
    }
}

static struct mini_block *find_free_block(size_t size)
{
    struct mini_block *block = block_head;

    while (block != (struct mini_block *)0) {
        if (block->is_free && block->size >= size) {
            return block;
        }
        block = block->next;
    }
    return (struct mini_block *)0;
}

static struct mini_block *grow_heap(size_t size)
{
    const size_t header_size = sizeof(struct mini_block);
    __UINTPTR_TYPE__ target;
    struct mini_block *block;
    long result;

    if (!initialize_heap()) {
        return (struct mini_block *)0;
    }
    if (size > (size_t)-1 - header_size) {
        return (struct mini_block *)0;
    }
    if (heap_end > (__UINTPTR_TYPE__)-1 - (__UINTPTR_TYPE__)(header_size + size)) {
        return (struct mini_block *)0;
    }

    target = heap_end + (__UINTPTR_TYPE__)(header_size + size);
    result = mini_sys_brk((void *)target);
    if ((__UINTPTR_TYPE__)result != target) {
        return (struct mini_block *)0;
    }

    block = (struct mini_block *)heap_end;
    block->size = size;
    block->next = (struct mini_block *)0;
    block->is_free = 0;
    block->reserved = 0;

    if (block_tail != (struct mini_block *)0) {
        block_tail->next = block;
    } else {
        block_head = block;
    }
    block_tail = block;
    heap_end = target;
    return block;
}

void *malloc(size_t size)
{
    size_t aligned;
    struct mini_block *block;

    if (size == 0) {
        return (void *)0;
    }
    if (!align_size(size, &aligned)) {
        errno = ENOMEM;
        return (void *)0;
    }

    block = find_free_block(aligned);
    if (block != (struct mini_block *)0) {
        split_block(block, aligned);
        block->is_free = 0;
        return (void *)(block + 1);
    }

    block = grow_heap(aligned);
    if (block == (struct mini_block *)0) {
        errno = ENOMEM;
        return (void *)0;
    }
    return (void *)(block + 1);
}

static int blocks_adjacent(const struct mini_block *left,
                           const struct mini_block *right)
{
    const unsigned char *end =
        (const unsigned char *)(left + 1) + left->size;

    return end == (const unsigned char *)right;
}

static void merge_with_next(struct mini_block *block)
{
    struct mini_block *next = block->next;

    if (next == (struct mini_block *)0 || !next->is_free ||
        !blocks_adjacent(block, next)) {
        return;
    }

    block->size += sizeof(struct mini_block) + next->size;
    block->next = next->next;
    if (block_tail == next) {
        block_tail = block;
    }
}

void free(void *ptr)
{
    struct mini_block *block;
    struct mini_block *previous = (struct mini_block *)0;
    struct mini_block *cursor;

    if (ptr == (void *)0) {
        return;
    }

    block = ((struct mini_block *)ptr) - 1;
    block->is_free = 1;
    merge_with_next(block);

    cursor = block_head;
    while (cursor != (struct mini_block *)0 && cursor != block) {
        previous = cursor;
        cursor = cursor->next;
    }

    if (previous != (struct mini_block *)0 && previous->is_free &&
        blocks_adjacent(previous, block)) {
        merge_with_next(previous);
    }
}
