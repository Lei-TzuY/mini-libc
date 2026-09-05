#include <errno.h>
#include <mini/syscall.h>
#include <stdlib.h>
#include <threads.h>

struct worker_arg {
    int loops;
    int errno_value;
};

static mtx_t lock;
static int counter;
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

int main(void)
{
    static struct worker_arg first = {1500, 51};
    static struct worker_arg second = {1500, 52};
    static const char marker[] = "tiny-threads-ok";
    thrd_t first_thread;
    thrd_t second_thread;
    thrd_t exit_thread;
    int first_result;
    int second_result;
    int exit_result;
    unsigned char *heap_check;

    main_thread = thrd_current();
    if (main_thread == (thrd_t)0 ||
        mtx_init(&lock, mtx_plain) != thrd_success) {
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

    heap_check = (unsigned char *)malloc(2048U);
    if (heap_check == (unsigned char *)0 || errno != EIO) {
        free(heap_check);
        return 4;
    }
    fill_bytes(heap_check, 2048U, 0x5aU);
    if (!bytes_match(heap_check, 2048U, 0x5aU)) {
        free(heap_check);
        return 5;
    }
    free(heap_check);

    if (thrd_create(&exit_thread, explicit_exit, (void *)0) != thrd_success ||
        thrd_join(exit_thread, &exit_result) != thrd_success ||
        exit_result != 91 || errno != EIO) {
        return 6;
    }

    mtx_destroy(&lock);
    if (mini_sys_write(1, marker, sizeof(marker) - 1U) !=
        (long)(sizeof(marker) - 1U)) {
        return 7;
    }
    return 0;
}
