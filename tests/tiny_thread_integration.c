#include <errno.h>
#include <mini/syscall.h>
#include <threads.h>

struct worker_arg {
    int loops;
    int errno_value;
};

static mtx_t lock;
static int counter;
static thrd_t main_thread;

static int worker(void *opaque)
{
    struct worker_arg *arg = (struct worker_arg *)opaque;
    int i;

    if (thrd_equal(thrd_current(), main_thread) || errno != 0) {
        return -1;
    }
    errno = arg->errno_value;
    for (i = 0; i < arg->loops; ++i) {
        if (mtx_lock(&lock) != thrd_success) {
            return -2;
        }
        ++counter;
        if (mtx_unlock(&lock) != thrd_success) {
            return -3;
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

    if (thrd_create(&exit_thread, explicit_exit, (void *)0) != thrd_success ||
        thrd_join(exit_thread, &exit_result) != thrd_success ||
        exit_result != 91 || errno != EIO) {
        return 4;
    }

    mtx_destroy(&lock);
    if (mini_sys_write(1, marker, sizeof(marker) - 1U) !=
        (long)(sizeof(marker) - 1U)) {
        return 5;
    }
    return 0;
}
