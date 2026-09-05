#include <errno.h>
#include <mini/syscall.h>
#include <threads.h>
#include <time.h>

static mtx_t timed_mutex;
static mtx_t owner_mutex;

static void add_milliseconds(struct timespec *time_point, long milliseconds)
{
    time_point->tv_nsec += milliseconds * 1000000L;
    if (time_point->tv_nsec >= 1000000000L) {
        ++time_point->tv_sec;
        time_point->tv_nsec -= 1000000000L;
    }
}

static int timeout_worker(void *opaque)
{
    struct timespec deadline;

    (void)opaque;
    if (timespec_get(&deadline, TIME_UTC) != TIME_UTC) {
        return 10;
    }
    add_milliseconds(&deadline, 20L);
    errno = ERANGE;
    if (mtx_timedlock(&timed_mutex, &deadline) != thrd_timedout ||
        errno != ERANGE) {
        return 11;
    }
    return 0;
}

static int wrong_unlock_worker(void *opaque)
{
    (void)opaque;
    return mtx_unlock(&owner_mutex) == thrd_error ? 0 : 20;
}

int main(void)
{
    static const char marker[] = "tiny-mutex-ok";
    mtx_t recursive;
    mtx_t expired;
    struct timespec past;
    thrd_t worker;
    int result;

    errno = EIO;
    if (mtx_init(&recursive, mtx_recursive | mtx_timed) != thrd_success ||
        mtx_lock(&recursive) != thrd_success ||
        mtx_lock(&recursive) != thrd_success || recursive.__depth != 2) {
        return 1;
    }

    past.tv_sec = -1;
    past.tv_nsec = 0L;
    if (mtx_timedlock(&recursive, &past) != thrd_success ||
        recursive.__depth != 3 ||
        mtx_unlock(&recursive) != thrd_success ||
        mtx_unlock(&recursive) != thrd_success ||
        mtx_unlock(&recursive) != thrd_success || errno != EIO) {
        return 2;
    }

    if (mtx_init(&expired, mtx_timed) != thrd_success ||
        mtx_timedlock(&expired, &past) != thrd_success ||
        mtx_unlock(&expired) != thrd_success || errno != EIO) {
        return 3;
    }

    if (mtx_init(&owner_mutex, mtx_plain) != thrd_success ||
        mtx_lock(&owner_mutex) != thrd_success ||
        thrd_create(&worker, wrong_unlock_worker, (void *)0) != thrd_success ||
        thrd_join(worker, &result) != thrd_success || result != 0 ||
        owner_mutex.__state != 1 ||
        mtx_unlock(&owner_mutex) != thrd_success || errno != EIO) {
        return 4;
    }

    if (mtx_init(&timed_mutex, mtx_timed) != thrd_success ||
        mtx_lock(&timed_mutex) != thrd_success ||
        thrd_create(&worker, timeout_worker, (void *)0) != thrd_success ||
        thrd_join(worker, &result) != thrd_success || result != 0 ||
        mtx_unlock(&timed_mutex) != thrd_success || errno != EIO) {
        return 5;
    }

    if (mini_sys_write(1, marker, sizeof(marker) - 1U) !=
        (long)(sizeof(marker) - 1U)) {
        return 6;
    }
    return 0;
}
