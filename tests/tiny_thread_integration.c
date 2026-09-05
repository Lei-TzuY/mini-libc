#include <errno.h>
#include <mini/syscall.h>
#include <stdlib.h>
#include <threads.h>

#define CAPACITY_THREADS 18
#define DETACHED_THREADS 8
#define MINI_FUTEX_WAIT 0
#define MINI_FUTEX_WAKE 1

struct worker_arg {
    int loops;
    int errno_value;
};

struct simple_worker_arg {
    int result;
};

static mtx_t lock;
static mtx_t gate;
static int counter;
static int detached_done;
static thrd_t main_thread;

static int bytes_match(const unsigned char *ptr, unsigned long size,
                       unsigned char pattern)
{
    unsigned long i;

    for (i = 0; i < size; ++i) {
        if (ptr[i] != pattern) {
            return 0;
        }
    }
    return 1;
}

static void fill_bytes(unsigned char *ptr, unsigned long size,
                       unsigned char pattern)
{
    unsigned long i;

    for (i = 0; i < size; ++i) {
        ptr[i] = pattern;
    }
}

static int worker(void *opaque)
{
    struct worker_arg *arg = (struct worker_arg *)opaque;
    int i;

    if (thrd_equal(thrd_current(), main_thread) || errno != 0) {
        return -1;
    }
    errno = arg->errno_value;
    for (i = 0; i < arg->loops; ++i) {
        unsigned long size = 23UL + (unsigned long)(i % 7) * 13UL;
        unsigned long grown = size + 29UL;
        unsigned char pattern =
            (unsigned char)(0x40U + (unsigned int)(arg->errno_value & 15) +
                            (unsigned int)(i & 3));
        unsigned char *memory = (unsigned char *)malloc(size);
        unsigned char *resized;

        if (memory == (unsigned char *)0) {
            return -2;
        }
        fill_bytes(memory, size, pattern);

        if ((i & 7) == 0) {
            unsigned int *zeroes =
                (unsigned int *)calloc(8U, sizeof(unsigned int));
            unsigned int j;

            if (zeroes == (unsigned int *)0) {
                free(memory);
                return -3;
            }
            for (j = 0; j < 8U; ++j) {
                if (zeroes[j] != 0U) {
                    free(zeroes);
                    free(memory);
                    return -4;
                }
            }
            free(zeroes);
        }

        resized = (unsigned char *)realloc(memory, grown);
        if (resized == (unsigned char *)0 ||
            !bytes_match(resized, size, pattern)) {
            if (resized != (unsigned char *)0) {
                free(resized);
            } else {
                free(memory);
            }
            return -5;
        }
        free(resized);

        if (mtx_lock(&lock) != thrd_success) {
            return -6;
        }
        ++counter;
        if (mtx_unlock(&lock) != thrd_success) {
            return -7;
        }
        if (errno != arg->errno_value) {
            return -8;
        }
    }
    return errno;
}

static int explicit_exit(void *opaque)
{
    (void)opaque;
    thrd_exit(91);
    return 0;
}

static int gated_worker(void *opaque)
{
    struct simple_worker_arg *arg = (struct simple_worker_arg *)opaque;

    if (mtx_lock(&gate) != thrd_success ||
        mtx_unlock(&gate) != thrd_success) {
        return -20;
    }
    return arg->result;
}

static int detached_worker(void *opaque)
{
    (void)opaque;

    if (mtx_lock(&gate) != thrd_success ||
        mtx_unlock(&gate) != thrd_success ||
        mtx_lock(&lock) != thrd_success) {
        return -30;
    }
    ++detached_done;
    if (mtx_unlock(&lock) != thrd_success) {
        return -31;
    }
    (void)mini_sys_futex((volatile int *)&detached_done, MINI_FUTEX_WAKE, 1,
                         (const void *)0, (volatile int *)0, 0);
    return 0;
}

static int wait_for_detached(void)
{
    for (;;) {
        int observed;

        if (mtx_lock(&lock) != thrd_success) {
            return 0;
        }
        observed = detached_done;
        if (mtx_unlock(&lock) != thrd_success) {
            return 0;
        }
        if (observed == DETACHED_THREADS) {
            return 1;
        }
        (void)mini_sys_futex((volatile int *)&detached_done, MINI_FUTEX_WAIT,
                             observed, (const void *)0,
                             (volatile int *)0, 0);
    }
}

int main(void)
{
    static struct worker_arg first = {1500, 51};
    static struct worker_arg second = {1500, 52};
    static const char marker[] = "tiny-threads-ok";
    struct simple_worker_arg capacity_args[CAPACITY_THREADS];
    thrd_t capacity_threads[CAPACITY_THREADS];
    thrd_t detached_threads[DETACHED_THREADS];
    thrd_t first_thread;
    thrd_t second_thread;
    thrd_t exit_thread;
    int first_result;
    int second_result;
    int exit_result;
    unsigned char *heap_check;
    int i;

    main_thread = thrd_current();
    if (main_thread == (thrd_t)0 ||
        mtx_init(&lock, mtx_plain) != thrd_success ||
        mtx_init(&gate, mtx_plain) != thrd_success) {
        return 1;
    }

    errno = EIO;
    if (thrd_create(&first_thread, worker, &first) != thrd_success ||
        thrd_create(&second_thread, worker, &second) != thrd_success ||
        errno != EIO) {
        return 2;
    }
    if (thrd_join(first_thread, &first_result) != thrd_success ||
        thrd_join(second_thread, &second_result) != thrd_success ||
        first_result != 51 || second_result != 52 || counter != 3000 ||
        errno != EIO) {
        return 3;
    }

    if (mtx_lock(&gate) != thrd_success) {
        return 4;
    }
    for (i = 0; i < CAPACITY_THREADS; ++i) {
        capacity_args[i].result = 100 + i;
        if (thrd_create(&capacity_threads[i], gated_worker, &capacity_args[i]) !=
                thrd_success ||
            errno != EIO) {
            return 5;
        }
    }
    if (mtx_unlock(&gate) != thrd_success) {
        return 6;
    }
    for (i = 0; i < CAPACITY_THREADS; ++i) {
        int result;

        if (thrd_join(capacity_threads[i], &result) != thrd_success ||
            result != capacity_args[i].result || errno != EIO) {
            return 7;
        }
    }

    detached_done = 0;
    if (mtx_lock(&gate) != thrd_success) {
        return 8;
    }
    for (i = 0; i < DETACHED_THREADS; ++i) {
        if (thrd_create(&detached_threads[i], detached_worker, (void *)0) !=
                thrd_success ||
            thrd_detach(detached_threads[i]) != thrd_success || errno != EIO) {
            return 9;
        }
    }
    if (thrd_join(detached_threads[0], (int *)0) != thrd_error ||
        thrd_detach(detached_threads[0]) != thrd_error || errno != EIO ||
        mtx_unlock(&gate) != thrd_success || !wait_for_detached()) {
        return 10;
    }

    heap_check = (unsigned char *)malloc(2048U);
    if (heap_check == (unsigned char *)0 || errno != EIO) {
        free(heap_check);
        return 11;
    }
    fill_bytes(heap_check, 2048U, 0x5aU);
    if (!bytes_match(heap_check, 2048U, 0x5aU)) {
        free(heap_check);
        return 12;
    }
    free(heap_check);

    if (thrd_create(&exit_thread, explicit_exit, (void *)0) != thrd_success ||
        thrd_join(exit_thread, &exit_result) != thrd_success ||
        exit_result != 91 || errno != EIO) {
        return 13;
    }

    mtx_destroy(&gate);
    mtx_destroy(&lock);
    if (mini_sys_write(1, marker, sizeof(marker) - 1U) !=
        (long)(sizeof(marker) - 1U)) {
        return 14;
    }
    return 0;
}
