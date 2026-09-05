#include <errno.h>
#include <time.h>

static long fake_result;
static struct timespec fake_value;
static int fake_clock_id;
static unsigned int fake_calls;

long mini_test_clock_gettime(int clockid, void *tp)
{
    struct timespec *out = (struct timespec *)tp;

    ++fake_calls;
    fake_clock_id = clockid;
    if (fake_result < 0) {
        return fake_result;
    }
    *out = fake_value;
    return fake_result;
}

static void reset_fake(long result, time_t sec, long nsec)
{
    fake_result = result;
    fake_value.tv_sec = sec;
    fake_value.tv_nsec = nsec;
    fake_clock_id = -1;
    fake_calls = 0U;
}

int main(void)
{
    const long max_long = (long)((~0UL) >> 1);
    time_t stored;
    struct timespec ts;
    clock_t ticks;
    double span;

    reset_fake(0, 123, 456789000L);
    stored = -9;
    errno = EIO;
    if (time(&stored) != 123 || stored != 123 || errno != EIO ||
        fake_calls != 1U || fake_clock_id != 0) {
        return 1;
    }

    reset_fake(0, 124, 1L);
    errno = EIO;
    if (time((time_t *)0) != 124 || errno != EIO || fake_calls != 1U ||
        fake_clock_id != 0) {
        return 2;
    }

    reset_fake(-ENOENT, 0, 0L);
    stored = 77;
    errno = EIO;
    if (time(&stored) != (time_t)-1 || stored != 77 || errno != ENOENT ||
        fake_calls != 1U || fake_clock_id != 0) {
        return 3;
    }

    reset_fake(0, 222, 333444555L);
    errno = EIO;
    if (timespec_get(&ts, TIME_UTC) != TIME_UTC || ts.tv_sec != 222 ||
        ts.tv_nsec != 333444555L || errno != EIO || fake_calls != 1U ||
        fake_clock_id != 0) {
        return 4;
    }

    reset_fake(0, 1, 2L);
    errno = EIO;
    if (timespec_get(&ts, 99) != 0 || errno != EINVAL || fake_calls != 0U) {
        return 5;
    }
    errno = EIO;
    if (timespec_get((struct timespec *)0, TIME_UTC) != 0 ||
        errno != EINVAL || fake_calls != 0U) {
        return 6;
    }

    reset_fake(-EIO, 0, 0L);
    errno = ENOENT;
    if (timespec_get(&ts, TIME_UTC) != 0 || errno != EIO ||
        fake_calls != 1U || fake_clock_id != 0) {
        return 7;
    }

    reset_fake(0, 2, 345678000L);
    errno = EIO;
    ticks = clock();
    if (ticks != 2345678L || errno != EIO || fake_calls != 1U ||
        fake_clock_id != 2) {
        return 8;
    }

    reset_fake(-ENOENT, 0, 0L);
    errno = EIO;
    if (clock() != (clock_t)-1 || errno != ENOENT || fake_calls != 1U ||
        fake_clock_id != 2) {
        return 9;
    }

    reset_fake(0, max_long / CLOCKS_PER_SEC + 1L, 0L);
    errno = EIO;
    if (clock() != (clock_t)-1 || errno != ERANGE || fake_calls != 1U ||
        fake_clock_id != 2) {
        return 10;
    }

    reset_fake(0, 1, 1000000000L);
    errno = EIO;
    if (clock() != (clock_t)-1 || errno != ERANGE || fake_calls != 1U) {
        return 11;
    }

    if (difftime((time_t)7, (time_t)2) != 5.0 ||
        difftime((time_t)2, (time_t)7) != -5.0) {
        return 12;
    }
    span = difftime((time_t)max_long, (time_t)(-max_long - 1L));
    if (!(span > 0.0)) {
        return 13;
    }

    return 0;
}
