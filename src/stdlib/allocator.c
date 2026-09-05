#include <errno.h>
#include <mini/syscall.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define MINI_ALLOC_ALIGNMENT 16UL
#define MINI_FUTEX_WAIT 0
#define MINI_FUTEX_WAKE 1

typedef unsigned long mini_uintptr_t;

struct mini_block {
    size_t size;
    struct mini_block *next;
    unsigned long is_free;
    size_t requested_size;
};

_Static_assert(sizeof(struct mini_block) == 32,
               "allocator header must preserve 16-byte payload alignment");

static struct mini_block *block_head;
static struct mini_block *block_tail;
static mini_uintptr_t heap_end;
static int heap_initialized;
static volatile int allocator_lock_word;

extern int __mini_atomic_exchange_int(volatile int *value, int replacement);

static void allocator_lock(void)
{
    while (__mini_atomic_exchange_int(&allocator_lock_word, 1) != 0) {
        (void)mini_sys_futex(&allocator_lock_word, MINI_FUTEX_WAIT, 1,
                             (const void *)0, (volatile int *)0, 0);
    }
}

static void allocator_unlock(void)
{
    if (__mini_atomic_exchange_int(&allocator_lock_word, 0) != 0) {
        (void)mini_sys_futex(&allocator_lock_word, MINI_FUTEX_WAKE, 1,
                             (const void *)0, (volatile int *)0, 0);
    }
}

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
    mini_uintptr_t current;
    mini_uintptr_t aligned;
    long result;

    if (heap_initialized) {
        return 1;
    }

    result = mini_sys_brk((void *)0);
    if (result <= 0) {
        return 0;
    }
    current = (mini_uintptr_t)result;
    if (current > (mini_uintptr_t)-1 - (MINI_ALLOC_ALIGNMENT - 1UL)) {
        return 0;
    }
    aligned = (current + (MINI_ALLOC_ALIGNMENT - 1UL)) &
              ~(mini_uintptr_t)(MINI_ALLOC_ALIGNMENT - 1UL);

    if (aligned != current) {
        result = mini_sys_brk((void *)aligned);
        if ((mini_uintptr_t)result != aligned) {
            return 0;
        }
    }

    heap_end = aligned;
    heap_initialized = 1;
    return 1;
}

static struct mini_block *split_block(struct mini_block *block, size_t size)
{
    size_t remaining = block->size - size;

    if (remaining < sizeof(struct mini_block) + MINI_ALLOC_ALIGNMENT) {
        return (struct mini_block *)0;
    }

    {
        struct mini_block *split =
            (struct mini_block *)((unsigned char *)(block + 1) + size);

        split->size = remaining - sizeof(struct mini_block);
        split->next = block->next;
        split->is_free = 1;
        split->requested_size = 0;
        block->size = size;
        block->next = split;
        if (block_tail == block) {
            block_tail = split;
        }
        return split;
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
    mini_uintptr_t target;
    struct mini_block *block;
    long result;

    if (!initialize_heap()) {
        return (struct mini_block *)0;
    }
    if (size > (size_t)-1 - header_size) {
        return (struct mini_block *)0;
    }
    if (heap_end > (mini_uintptr_t)-1 - (mini_uintptr_t)(header_size + size)) {
        return (struct mini_block *)0;
    }

    target = heap_end + (mini_uintptr_t)(header_size + size);
    result = mini_sys_brk((void *)target);
    if ((mini_uintptr_t)result != target) {
        return (struct mini_block *)0;
    }

    block = (struct mini_block *)heap_end;
    block->size = size;
    block->next = (struct mini_block *)0;
    block->is_free = 0;
    block->requested_size = 0;

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

    allocator_lock();
    block = find_free_block(aligned);
    if (block != (struct mini_block *)0) {
        (void)split_block(block, aligned);
        block->is_free = 0;
        block->requested_size = size;
        allocator_unlock();
        return (void *)(block + 1);
    }

    block = grow_heap(aligned);
    if (block != (struct mini_block *)0) {
        block->requested_size = size;
        allocator_unlock();
        return (void *)(block + 1);
    }
    allocator_unlock();
    errno = ENOMEM;
    return (void *)0;
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

    allocator_lock();
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
    allocator_unlock();
}

void *realloc(void *ptr, size_t size)
{
    struct mini_block *block;
    struct mini_block *next;
    struct mini_block *split;
    size_t aligned;
    size_t combined;
    size_t copy_size;
    void *replacement;

    if (ptr == (void *)0) {
        return malloc(size);
    }
    if (size == 0) {
        free(ptr);
        return (void *)0;
    }

    if (!align_size(size, &aligned)) {
        errno = ENOMEM;
        return (void *)0;
    }

    allocator_lock();
    block = ((struct mini_block *)ptr) - 1;

    if (aligned <= block->size) {
        split = split_block(block, aligned);
        if (split != (struct mini_block *)0) {
            merge_with_next(split);
        }
        block->requested_size = size;
        allocator_unlock();
        return ptr;
    }

    next = block->next;
    if (next != (struct mini_block *)0 && next->is_free &&
        blocks_adjacent(block, next) &&
        next->size <= (size_t)-1 - sizeof(struct mini_block) &&
        block->size <= (size_t)-1 - sizeof(struct mini_block) - next->size) {
        combined = block->size + sizeof(struct mini_block) + next->size;
        if (combined >= aligned) {
            merge_with_next(block);
            split = split_block(block, aligned);
            if (split != (struct mini_block *)0) {
                merge_with_next(split);
            }
            block->requested_size = size;
            allocator_unlock();
            return ptr;
        }

        if (next == block_tail) {
            size_t growth = aligned - combined;

            if (heap_end <= (mini_uintptr_t)-1 - (mini_uintptr_t)growth) {
                mini_uintptr_t target = heap_end + (mini_uintptr_t)growth;
                long result = mini_sys_brk((void *)target);

                if ((mini_uintptr_t)result == target) {
                    heap_end = target;
                    merge_with_next(block);
                    block->size = aligned;
                    block_tail = block;
                    block->requested_size = size;
                    allocator_unlock();
                    return ptr;
                }
            }
        }
    }

    if (block == block_tail) {
        size_t growth = aligned - block->size;

        if (heap_end <= (mini_uintptr_t)-1 - (mini_uintptr_t)growth) {
            mini_uintptr_t target = heap_end + (mini_uintptr_t)growth;
            long result = mini_sys_brk((void *)target);

            if ((mini_uintptr_t)result == target) {
                heap_end = target;
                block->size = aligned;
                block->requested_size = size;
                allocator_unlock();
                return ptr;
            }
        }
    }

    copy_size = block->requested_size < size ? block->requested_size : size;
    allocator_unlock();

    replacement = malloc(size);
    if (replacement == (void *)0) {
        return (void *)0;
    }

    memcpy(replacement, ptr, copy_size);
    free(ptr);
    return replacement;
}
