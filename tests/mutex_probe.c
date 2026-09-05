#include <errno.h>
#include <stdio.h>
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
    add_milliseconds(&deadline, 25L);
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
    errno = ERANGE;
    if (mtx_unlock(&owner_mutex) != thrd_error || errno != ERANGE) {
        return 20;
    }
    return 0;
}

int main(void)
{
    mtx_t recursive;
    mtx_t recursive_timed;
    mtx_t expired_free;
    struct timespec past;
    thrd_t worker;
    int result;

    errno = EIO;
    if (mtx_init(&recursive, mtx_recursive) != thrd_success || errno != EIO ||
        mtx_lock(&recursive) != thrd_success ||
        mtx_lock(&recursive) != thrd_success || recursive.__depth != 2 ||
        mtx_trylock(&recursive) != thrd_success || recursive.__depth != 3 ||
        mtx_unlock(&recursive) != thrd_success || recursive.__depth != 2 ||
        mtx_unlock(&recursive) != thrd_success || recursive.__depth != 1 ||
        mtx_unlock(&recursive) != thrd_success || recursive.__state != 0 ||
        errno != EIO) {
        return 1;
    }

    past.tv_sec = -1;
    past.tv_nsec = 0L;
    if (mtx_init(&expired_free, mtx_timed) != thrd_success ||
        mtx_timedlock(&expired_free, &past) != thrd_success ||
        mtx_unlock(&expired_free) != thrd_success || errno != EIO) {
        return 2;
    }

    if (mtx_init(&recursive_timed, mtx_recursive | mtx_timed) != thrd_success ||
        mtx_lock(&recursive_timed) != thrd_success ||
        mtx_timedlock(&recursive_timed, &past) != thrd_success ||
        recursive_timed.__depth != 2 ||
        mtx_unlock(&recursive_timed) != thrd_success ||
        recursive_timed.__depth != 1 ||
        mtx_unlock(&recursive_timed) != thrd_success || errno != EIO) {
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
        timed_mutex.__state != 1 ||
        mtx_unlock(&timed_mutex) != thrd_success || errno != EIO) {
        return 5;
    }

    if (mtx_init(&timed_mutex, mtx_plain) != thrd_success ||
        mtx_timedlock(&timed_mutex, &past) != thrd_error || errno != EIO) {
        return 6;
    }

    mtx_destroy(&recursive);
    mtx_destroy(&recursive_timed);
    mtx_destroy(&expired_free);
    mtx_destroy(&owner_mutex);
    mtx_destroy(&timed_mutex);
    if (errno != EIO) {
        return 7;
    }

    if (puts("mutex-types-ok") == EOF) {
        return 8;
    }
    return 0;
}
