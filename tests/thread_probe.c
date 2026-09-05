#include <errno.h>
#include <mini/syscall.h>
#include <stdlib.h>
#include <threads.h>

struct worker_arg {
    int loops;
    int errno_value;
};

struct allocator_worker_arg {
    int loops;
    int id;
    int errno_value;
};

static mtx_t counter_mutex;
static int shared_counter;
static thrd_t main_thread;

static int counter_worker(void *opaque)
{
    struct worker_arg *arg = (struct worker_arg *)opaque;
    int i;

    if (thrd_equal(thrd_current(), main_thread) || errno != 0) {
        return -1;
    }
    errno = arg->errno_value;

    for (i = 0; i < arg->loops; ++i) {
        if (mtx_lock(&counter_mutex) != thrd_success) {
            return -2;
        }
        ++shared_counter;
        if (mtx_unlock(&counter_mutex) != thrd_success) {
            return -3;
        }
    }
    return errno;
}

static int check_pattern(const unsigned char *ptr, unsigned long size,
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

static void fill_pattern(unsigned char *ptr, unsigned long size,
                         unsigned char pattern)
{
    unsigned long i;

    for (i = 0; i < size; ++i) {
        ptr[i] = pattern;
    }
}

static int allocator_worker(void *opaque)
{
    struct allocator_worker_arg *arg = (struct allocator_worker_arg *)opaque;
    int i;

    if (errno != 0) {
        return -10;
    }
    errno = arg->errno_value;

    for (i = 0; i < arg->loops; ++i) {
        unsigned long size =
            31UL + (unsigned long)((i + arg->id * 3) % 9) * 17UL;
        unsigned long grown = size + 37UL + (unsigned long)(i % 23);
        unsigned long shrunk = size / 2UL + 1UL;
        unsigned char pattern =
            (unsigned char)(0x31U + (unsigned int)arg->id +
                            (unsigned int)(i & 7));
        unsigned char *ptr = (unsigned char *)malloc(size);
        unsigned char *resized;

        if (ptr == (unsigned char *)0) {
            return -11;
        }
        fill_pattern(ptr, size, pattern);

        if ((i & 3) == 0) {
            unsigned int *zeros = (unsigned int *)calloc(16U, sizeof(unsigned int));
            unsigned int j;

            if (zeros == (unsigned int *)0) {
                free(ptr);
                return -12;
            }
            for (j = 0; j < 16U; ++j) {
                if (zeros[j] != 0U) {
                    free(zeros);
                    free(ptr);
                    return -13;
                }
            }
            free(zeros);
        }

        resized = (unsigned char *)realloc(ptr, grown);
        if (resized == (unsigned char *)0 ||
            !check_pattern(resized, size, pattern)) {
            if (resized != (unsigned char *)0) {
                free(resized);
            } else {
                free(ptr);
            }
            return -14;
        }
        fill_pattern(resized + size, grown - size, pattern);

        ptr = (unsigned char *)realloc(resized, shrunk);
        if (ptr == (unsigned char *)0 ||
            !check_pattern(ptr, shrunk, pattern)) {
            if (ptr != (unsigned char *)0) {
                free(ptr);
            } else {
                free(resized);
            }
            return -15;
        }
        free(ptr);

        if (errno != arg->errno_value) {
            return -16;
        }
    }
    return errno;
}

static int exit_worker(void *opaque)
{
    (void)opaque;

    if (errno != 0) {
        thrd_exit(-4);
    }
    errno = ERANGE;
    thrd_exit(73);
    return 0;
}

int main(void)
{
    static struct worker_arg first = {4000, 41};
    static struct worker_arg second = {4000, 42};
    static struct allocator_worker_arg alloc_first = {600, 1, 61};
    static struct allocator_worker_arg alloc_second = {600, 2, 62};
    static struct allocator_worker_arg alloc_third = {600, 3, 63};
    static struct allocator_worker_arg alloc_fourth = {600, 4, 64};
    static const char marker[] = "threads-ok";
    thrd_t first_thread;
    thrd_t second_thread;
    thrd_t exit_thread;
    thrd_t alloc_thread_one;
    thrd_t alloc_thread_two;
    thrd_t alloc_thread_three;
    thrd_t alloc_thread_four;
    int first_result;
    int second_result;
    int exit_result;
    int alloc_result_one;
    int alloc_result_two;
    int alloc_result_three;
    int alloc_result_four;
    unsigned char *heap_check;

    main_thread = thrd_current();
    if (main_thread == (thrd_t)0 || !thrd_equal(main_thread, thrd_current())) {
        return 1;
    }

    if (mtx_init(&counter_mutex, mtx_plain) != thrd_success ||
        mtx_lock(&counter_mutex) != thrd_success ||
        mtx_trylock(&counter_mutex) != thrd_busy ||
        mtx_unlock(&counter_mutex) != thrd_success) {
        return 2;
    }

    errno = EIO;
    if (thrd_create(&first_thread, counter_worker, &first) != thrd_success ||
        thrd_create(&second_thread, counter_worker, &second) != thrd_success ||
        errno != EIO) {
        return 3;
    }
    if (thrd_equal(first_thread, second_thread) ||
        thrd_equal(first_thread, main_thread) ||
        thrd_equal(second_thread, main_thread)) {
        return 4;
    }

    if (thrd_join(first_thread, &first_result) != thrd_success ||
        thrd_join(second_thread, &second_result) != thrd_success ||
        first_result != first.errno_value || second_result != second.errno_value ||
        shared_counter != first.loops + second.loops || errno != EIO) {
        return 5;
    }

    if (thrd_create(&alloc_thread_one, allocator_worker, &alloc_first) !=
            thrd_success ||
        thrd_create(&alloc_thread_two, allocator_worker, &alloc_second) !=
            thrd_success ||
        thrd_create(&alloc_thread_three, allocator_worker, &alloc_third) !=
            thrd_success ||
        thrd_create(&alloc_thread_four, allocator_worker, &alloc_fourth) !=
            thrd_success ||
        errno != EIO) {
        return 6;
    }
    if (thrd_join(alloc_thread_one, &alloc_result_one) != thrd_success ||
        thrd_join(alloc_thread_two, &alloc_result_two) != thrd_success ||
        thrd_join(alloc_thread_three, &alloc_result_three) != thrd_success ||
        thrd_join(alloc_thread_four, &alloc_result_four) != thrd_success ||
        alloc_result_one != alloc_first.errno_value ||
        alloc_result_two != alloc_second.errno_value ||
        alloc_result_three != alloc_third.errno_value ||
        alloc_result_four != alloc_fourth.errno_value || errno != EIO) {
        return 7;
    }

    heap_check = (unsigned char *)malloc(4096U);
    if (heap_check == (unsigned char *)0 || errno != EIO) {
        free(heap_check);
        return 8;
    }
    fill_pattern(heap_check, 4096U, 0xa5U);
    if (!check_pattern(heap_check, 4096U, 0xa5U)) {
        free(heap_check);
        return 9;
    }
    free(heap_check);

    if (thrd_create(&exit_thread, exit_worker, (void *)0) != thrd_success ||
        thrd_join(exit_thread, &exit_result) != thrd_success ||
        exit_result != 73 || errno != EIO) {
        return 10;
    }

    if (thrd_join(first_thread, (int *)0) != thrd_error ||
        mtx_unlock(&counter_mutex) != thrd_error || errno != EIO) {
        return 11;
    }

    mtx_destroy(&counter_mutex);
    if (mini_sys_write(1, marker, sizeof(marker) - 1U) !=
        (long)(sizeof(marker) - 1U)) {
        return 12;
    }
    return 0;
}
