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

static int same_string(const char *lhs, const char *rhs)
{
    while (*lhs == *rhs) {
        if (*lhs == '\0') {
            return 1;
        }
        ++lhs;
        ++rhs;
    }
    return 0;
}

int main(void)
{
    static const char expected_calendar[] =
        "Tue Tuesday Feb February 2000-02-29 00:00:00 060 2 09 09 2000-W09-2 +0000 UTC";
    const long max_long = (long)((~0UL) >> 1);
    time_t stored;
    time_t stamp;
    time_t leap = 951782400L;
    time_t epoch = 0L;
    time_t negative = -1L;
    struct timespec ts;
    struct tm *broken;
    struct tm value;
    char buffer[160];
    char small[5];
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

    reset_fake(0, 999, 999L);
    errno = EIO;
    broken = gmtime(&epoch);
    if (broken == (struct tm *)0 || broken->tm_year != 70 ||
        broken->tm_mon != 0 || broken->tm_mday != 1 ||
        broken->tm_wday != 4 || broken->tm_yday != 0 ||
        errno != EIO || fake_calls != 0U) {
        return 14;
    }
    broken = gmtime(&negative);
    if (broken == (struct tm *)0 || broken->tm_year != 69 ||
        broken->tm_mon != 11 || broken->tm_mday != 31 ||
        broken->tm_hour != 23 || broken->tm_min != 59 ||
        broken->tm_sec != 59 || broken->tm_wday != 3 ||
        broken->tm_yday != 364 || fake_calls != 0U) {
        return 15;
    }

    broken = localtime(&leap);
    if (broken == (struct tm *)0 || broken->tm_year != 100 ||
        broken->tm_mon != 1 || broken->tm_mday != 29 ||
        broken->tm_wday != 2 || broken->tm_yday != 59 ||
        broken->tm_isdst != 0 || fake_calls != 0U || errno != EIO) {
        return 16;
    }
    value = *broken;
    if (!same_string(asctime(&value), "Tue Feb 29 00:00:00 2000\n") ||
        !same_string(ctime(&leap), "Tue Feb 29 00:00:00 2000\n") ||
        fake_calls != 0U || errno != EIO) {
        return 17;
    }
    if (strftime(buffer, sizeof(buffer),
                 "%a %A %b %B %F %T %j %w %U %W %G-W%V-%u %z %Z",
                 &value) != sizeof(expected_calendar) - 1U ||
        !same_string(buffer, expected_calendar) || fake_calls != 0U ||
        errno != EIO) {
        return 18;
    }

    value.tm_sec = 70;
    value.tm_min = 59;
    value.tm_hour = 23;
    value.tm_mday = 31;
    value.tm_mon = 11;
    value.tm_year = 99;
    value.tm_wday = 0;
    value.tm_yday = 0;
    value.tm_isdst = 1;
    stamp = mktime(&value);
    if (stamp != 946684810L || value.tm_year != 100 || value.tm_mon != 0 ||
        value.tm_mday != 1 || value.tm_hour != 0 || value.tm_min != 0 ||
        value.tm_sec != 10 || value.tm_wday != 6 || value.tm_yday != 0 ||
        value.tm_isdst != 0 || fake_calls != 0U || errno != EIO) {
        return 19;
    }

    small[0] = 'Q';
    small[1] = 'Q';
    small[2] = 'Q';
    small[3] = 'Q';
    small[4] = 'Q';
    if (strftime(small, sizeof(small), "%F", &value) != 0U ||
        small[4] != '\0' || fake_calls != 0U || errno != EIO) {
        return 20;
    }
    errno = EIO;
    if (strftime(buffer, sizeof(buffer), "%q", &value) != 0U ||
        errno != EINVAL || fake_calls != 0U) {
        return 21;
    }

    value.tm_sec = 0;
    value.tm_min = 0;
    value.tm_hour = 0;
    value.tm_mday = 1;
    value.tm_mon = 0;
    value.tm_year = -1900;
    value.tm_wday = 6;
    value.tm_yday = 0;
    value.tm_isdst = 0;
    errno = EIO;
    if (strftime(buffer, sizeof(buffer), "%G", &value) != 0U ||
        errno != EINVAL || fake_calls != 0U) {
        return 22;
    }
    errno = EIO;
    if (strftime(buffer, sizeof(buffer), "%g", &value) != 0U ||
        errno != EINVAL || fake_calls != 0U) {
        return 23;
    }

    return 0;
}
