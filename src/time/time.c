#include <errno.h>
#include <mini/syscall.h>
#include <time.h>

#define MINI_CLOCK_REALTIME 0
#define MINI_CLOCK_PROCESS_CPUTIME_ID 2
#define MINI_LONG_MAX ((long)((~0UL) >> 1))

static int sample_clock(int clock_id, struct timespec *ts)
{
    long result = mini_sys_clock_gettime(clock_id, ts);

    if (result < 0) {
        errno = (int)(-result);
        return 0;
    }
    return 1;
}

time_t time(time_t *timer)
{
    struct timespec ts;

    if (!sample_clock(MINI_CLOCK_REALTIME, &ts)) {
        return (time_t)-1;
    }
    if (timer != (time_t *)0) {
        *timer = ts.tv_sec;
    }
    return ts.tv_sec;
}

int timespec_get(struct timespec *ts, int base)
{
    if (ts == (struct timespec *)0 || base != TIME_UTC) {
        errno = EINVAL;
        return 0;
    }
    if (!sample_clock(MINI_CLOCK_REALTIME, ts)) {
        return 0;
    }
    return TIME_UTC;
}

clock_t clock(void)
{
    struct timespec ts;
    long seconds_part;
    long micros_part;

    if (!sample_clock(MINI_CLOCK_PROCESS_CPUTIME_ID, &ts)) {
        return (clock_t)-1;
    }
    if (ts.tv_sec < 0 || ts.tv_nsec < 0 || ts.tv_nsec >= 1000000000L ||
        ts.tv_sec > MINI_LONG_MAX / CLOCKS_PER_SEC) {
        errno = ERANGE;
        return (clock_t)-1;
    }

    seconds_part = ts.tv_sec * CLOCKS_PER_SEC;
    micros_part = ts.tv_nsec / 1000L;
    if (micros_part > MINI_LONG_MAX - seconds_part) {
        errno = ERANGE;
        return (clock_t)-1;
    }
    return (clock_t)(seconds_part + micros_part);
}

double difftime(time_t time1, time_t time0)
{
    return (double)time1 - (double)time0;
}
