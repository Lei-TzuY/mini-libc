#include <errno.h>
#include <mini/syscall.h>
#include <stdlib.h>
#include <threads.h>

#define CAPACITY_THREADS 24
#define DETACHED_THREADS 24
#define MINI_FUTEX_WAIT 0
#define MINI_FUTEX_WAKE 1
#define WAIT_ATTEMPTS 1000

struct worker_arg {
    int loops;
    int errno_value;
};

struct allocator_worker_arg {
    int loops;
    int id;
    int errno_value;
};

struct simple_worker_arg {
    int result;
};

struct race_join_arg {
    thrd_t target;
    int target_result;
};

struct mini_timeout {
    long tv_sec;
    long tv_nsec;
};

static mtx_t counter_mutex;
static mtx_t capacity_gate;
static mtx_t detach_gate;
static mtx_t race_gate;
static int shared_counter;
static int detached_done;
static int race_target_done;
static int self_detach_done;
static int self_detach_status;
static thrd_t main_thread;
static const struct mini_timeout wait_slice = {0L, 10000000L};

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

static int gated_worker(void *opaque)
{
    struct simple_worker_arg *arg = (struct simple_worker_arg *)opaque;

    if (mtx_lock(&capacity_gate) != thrd_success ||
        mtx_unlock(&capacity_gate) != thrd_success) {
        return -20;
    }
    return arg->result;
}

static int detached_worker(void *opaque)
{
    (void)opaque;

    if (mtx_lock(&detach_gate) != thrd_success ||
        mtx_unlock(&detach_gate) != thrd_success ||
        mtx_lock(&counter_mutex) != thrd_success) {
        return -30;
    }
    ++detached_done;
    if (mtx_unlock(&counter_mutex) != thrd_success) {
        return -31;
    }
    (void)mini_sys_futex((volatile int *)&detached_done, MINI_FUTEX_WAKE, 1,
                         (const void *)0, (volatile int *)0, 0);
    return 0;
}

static int race_target_worker(void *opaque)
{
    (void)opaque;

    if (mtx_lock(&race_gate) != thrd_success ||
        mtx_unlock(&race_gate) != thrd_success ||
        mtx_lock(&counter_mutex) != thrd_success) {
        return -40;
    }
    race_target_done = 1;
    if (mtx_unlock(&counter_mutex) != thrd_success) {
        return -41;
    }
    (void)mini_sys_futex((volatile int *)&race_target_done, MINI_FUTEX_WAKE, 1,
                         (const void *)0, (volatile int *)0, 0);
    return 77;
}

static int race_join_worker(void *opaque)
{
    struct race_join_arg *arg = (struct race_join_arg *)opaque;

    arg->target_result = -1;
    return thrd_join(arg->target, &arg->target_result);
}

static int race_detach_worker(void *opaque)
{
    thrd_t target = *(thrd_t *)opaque;

    return thrd_detach(target);
}

static int self_detach_worker(void *opaque)
{
    int status;

    (void)opaque;
    status = thrd_detach(thrd_current());
    if (mtx_lock(&counter_mutex) != thrd_success) {
        return -50;
    }
    self_detach_status = status;
    self_detach_done = 1;
    if (mtx_unlock(&counter_mutex) != thrd_success) {
        return -51;
    }
    (void)mini_sys_futex((volatile int *)&self_detach_done, MINI_FUTEX_WAKE, 1,
                         (const void *)0, (volatile int *)0, 0);
    return status == thrd_success ? 88 : -52;
}

static int wait_for_count(int *value, int expected)
{
    int attempt;

    for (attempt = 0; attempt < WAIT_ATTEMPTS; ++attempt) {
        int observed;

        if (mtx_lock(&counter_mutex) != thrd_success) {
            return 0;
        }
        observed = *value;
        if (mtx_unlock(&counter_mutex) != thrd_success) {
            return 0;
        }
        if (observed == expected) {
            return 1;
        }
        (void)mini_sys_futex((volatile int *)value, MINI_FUTEX_WAIT, observed,
                             &wait_slice, (volatile int *)0, 0);
    }
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
    struct simple_worker_arg capacity_args[CAPACITY_THREADS];
    thrd_t capacity_threads[CAPACITY_THREADS];
    thrd_t detached_threads[DETACHED_THREADS];
    thrd_t first_thread;
    thrd_t second_thread;
    thrd_t exit_thread;
    thrd_t alloc_thread_one;
    thrd_t alloc_thread_two;
    thrd_t alloc_thread_three;
    thrd_t alloc_thread_four;
    thrd_t race_target;
    thrd_t race_joiner;
    thrd_t race_detacher;
    thrd_t self_detached;
    struct race_join_arg race_join_arg;
    int first_result;
    int second_result;
    int exit_result;
    int alloc_result_one;
    int alloc_result_two;
    int alloc_result_three;
    int alloc_result_four;
    int race_join_status;
    int race_detach_status;
    unsigned char *heap_check;
    int i;

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

    if (mtx_init(&capacity_gate, mtx_plain) != thrd_success ||
        mtx_lock(&capacity_gate) != thrd_success) {
        return 8;
    }
    for (i = 0; i < CAPACITY_THREADS; ++i) {
        capacity_args[i].result = 100 + i;
        if (thrd_create(&capacity_threads[i], gated_worker, &capacity_args[i]) !=
                thrd_success ||
            errno != EIO) {
            return 9;
        }
    }
    if (mtx_unlock(&capacity_gate) != thrd_success) {
        return 10;
    }
    for (i = 0; i < CAPACITY_THREADS; ++i) {
        int result;

        if (thrd_join(capacity_threads[i], &result) != thrd_success ||
            result != capacity_args[i].result || errno != EIO) {
            return 11;
        }
    }
    mtx_destroy(&capacity_gate);

    if (mtx_init(&detach_gate, mtx_plain) != thrd_success ||
        mtx_lock(&detach_gate) != thrd_success) {
        return 12;
    }
    detached_done = 0;
    for (i = 0; i < DETACHED_THREADS; ++i) {
        if (thrd_create(&detached_threads[i], detached_worker, (void *)0) !=
                thrd_success ||
            thrd_detach(detached_threads[i]) != thrd_success || errno != EIO) {
            return 13;
        }
    }
    if (thrd_join(detached_threads[0], (int *)0) != thrd_error ||
        thrd_detach(detached_threads[0]) != thrd_error || errno != EIO) {
        return 14;
    }
    if (mtx_unlock(&detach_gate) != thrd_success ||
        !wait_for_count(&detached_done, DETACHED_THREADS)) {
        return 15;
    }
    mtx_destroy(&detach_gate);

    race_target_done = 0;
    if (mtx_init(&race_gate, mtx_plain) != thrd_success ||
        mtx_lock(&race_gate) != thrd_success ||
        thrd_create(&race_target, race_target_worker, (void *)0) != thrd_success) {
        return 16;
    }
    race_join_arg.target = race_target;
    race_join_arg.target_result = -1;
    if (thrd_create(&race_joiner, race_join_worker, &race_join_arg) != thrd_success ||
        thrd_create(&race_detacher, race_detach_worker, &race_target) !=
            thrd_success ||
        mtx_unlock(&race_gate) != thrd_success) {
        return 17;
    }
    if (thrd_join(race_joiner, &race_join_status) != thrd_success ||
        thrd_join(race_detacher, &race_detach_status) != thrd_success ||
        !wait_for_count(&race_target_done, 1) ||
        ((race_join_status == thrd_success) ==
         (race_detach_status == thrd_success)) ||
        (race_join_status == thrd_success && race_join_arg.target_result != 77) ||
        (race_join_status != thrd_success && race_join_status != thrd_error) ||
        (race_detach_status != thrd_success && race_detach_status != thrd_error) ||
        errno != EIO) {
        return 18;
    }
    mtx_destroy(&race_gate);

    self_detach_done = 0;
    self_detach_status = thrd_error;
    if (thrd_create(&self_detached, self_detach_worker, (void *)0) != thrd_success ||
        !wait_for_count(&self_detach_done, 1) ||
        self_detach_status != thrd_success ||
        thrd_join(self_detached, (int *)0) != thrd_error || errno != EIO) {
        return 19;
    }

    heap_check = (unsigned char *)malloc(4096U);
    if (heap_check == (unsigned char *)0 || errno != EIO) {
        free(heap_check);
        return 20;
    }
    fill_pattern(heap_check, 4096U, 0xa5U);
    if (!check_pattern(heap_check, 4096U, 0xa5U)) {
        free(heap_check);
        return 21;
    }
    free(heap_check);

    if (thrd_create(&exit_thread, exit_worker, (void *)0) != thrd_success ||
        thrd_join(exit_thread, &exit_result) != thrd_success ||
        exit_result != 73 || errno != EIO) {
        return 22;
    }

    if (thrd_join(first_thread, (int *)0) != thrd_error ||
        mtx_unlock(&counter_mutex) != thrd_error || errno != EIO) {
        return 23;
    }

    mtx_destroy(&counter_mutex);
    if (mini_sys_write(1, marker, sizeof(marker) - 1U) !=
        (long)(sizeof(marker) - 1U)) {
        return 24;
    }
    return 0;
}
