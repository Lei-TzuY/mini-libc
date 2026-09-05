#include <errno.h>
#include <mini/syscall.h>
#include <threads.h>

struct worker_arg {
    int loops;
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
    static const char marker[] = "threads-ok";
    thrd_t first_thread;
    thrd_t second_thread;
    thrd_t exit_thread;
    int first_result;
    int second_result;
    int exit_result;

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

    if (thrd_create(&exit_thread, exit_worker, (void *)0) != thrd_success ||
        thrd_join(exit_thread, &exit_result) != thrd_success ||
        exit_result != 73 || errno != EIO) {
        return 6;
    }

    if (thrd_join(first_thread, (int *)0) != thrd_error ||
        mtx_unlock(&counter_mutex) != thrd_error || errno != EIO) {
        return 7;
    }

    mtx_destroy(&counter_mutex);
    if (mini_sys_write(1, marker, sizeof(marker) - 1U) !=
        (long)(sizeof(marker) - 1U)) {
        return 8;
    }
    return 0;
}
