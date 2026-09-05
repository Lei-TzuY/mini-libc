#include <errno.h>
#include <mini/syscall.h>
#include <threads.h>

#define MINI_RAW_EINTR (-4L)
#define MINI_NSEC_PER_SEC 1000000000L
#define MINI_THRD_SLEEP_ERROR (-2)

static int valid_duration(const struct timespec *duration)
{
    return duration != (const struct timespec *)0 &&
           duration->tv_sec >= 0 &&
           duration->tv_nsec >= 0L &&
           duration->tv_nsec < MINI_NSEC_PER_SEC;
}

int thrd_sleep(const struct timespec *duration, struct timespec *remaining)
{
    int saved_errno = errno;
    long result;

    if (!valid_duration(duration)) {
        errno = saved_errno;
        return MINI_THRD_SLEEP_ERROR;
    }

    result = mini_sys_nanosleep((const void *)duration, (void *)remaining);
    errno = saved_errno;
    if (result == 0L) {
        return 0;
    }
    if (result == MINI_RAW_EINTR) {
        return -1;
    }
    return MINI_THRD_SLEEP_ERROR;
}
