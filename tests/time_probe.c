#include <errno.h>
#include <mini/syscall.h>
#include <threads.h>
#include <time.h>

struct calendar_worker_result {
    struct tm *tm_slot;
    char *text_slot;
    int year;
    int month;
    int day;
};

static struct calendar_worker_result worker_result;

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

static int calendar_worker(void *arg)
{
    time_t leap = 951782400L;
    struct tm *broken;
    char *text;

    (void)arg;
    broken = gmtime(&leap);
    if (broken == (struct tm *)0 || broken->tm_year != 100 ||
        broken->tm_mon != 1 || broken->tm_mday != 29) {
        return 1;
    }
    text = asctime(broken);
    if (text == (char *)0 ||
        !same_string(text, "Tue Feb 29 00:00:00 2000\n")) {
        return 2;
    }
    worker_result.tm_slot = broken;
    worker_result.text_slot = text;
    worker_result.year = broken->tm_year;
    worker_result.month = broken->tm_mon;
    worker_result.day = broken->tm_mday;
    return 0;
}

int main(void)
{
    static const char ok[] = "time-ok\n";
    static const char expected_calendar[] =
        "Tue|Tuesday|Feb|February|2000-02-29|00:00:00|060|2|2|09|09|"
        "2000-W09-2|+0000|UTC|Tue Feb 29 00:00:00 2000";
    struct timespec realtime;
    struct tm leap_tm;
    struct tm normalized;
    struct tm *broken;
    struct tm *main_tm_slot;
    char *main_text_slot;
    char calendar[192];
    char small[5];
    time_t wall;
    time_t stored;
    time_t stamp;
    time_t leap = 951782400L;
    time_t epoch = 0L;
    time_t before_epoch = -1L;
    time_t nonleap = 4107542400L;
    time_t boundary = 2147483647L;
    time_t extreme = (time_t)((~0UL) >> 1);
    clock_t before;
    clock_t after;
    double delta;
    volatile unsigned long work = 0UL;
    unsigned long i;
    thrd_t worker;
    int worker_status;

    errno = EIO;
    if (timespec_get(&realtime, TIME_UTC) != TIME_UTC ||
        realtime.tv_sec <= 0 || realtime.tv_nsec < 0 ||
        realtime.tv_nsec >= 1000000000L || errno != EIO) {
        return 1;
    }

    stored = 0;
    if ((wall = time(&stored)) == (time_t)-1 || stored != wall ||
        errno != EIO) {
        return 2;
    }
    delta = difftime(wall, realtime.tv_sec);
    if (delta < -2.0 || delta > 2.0) {
        return 3;
    }

    before = clock();
    if (before == (clock_t)-1 || before < 0 || errno != EIO) {
        return 4;
    }
    for (i = 0UL; i < 200000UL; ++i) {
        work += i ^ (work >> 3);
    }
    after = clock();
    if (after == (clock_t)-1 || after < before || errno != EIO) {
        return 5;
    }
    (void)work;

    errno = EIO;
    if (timespec_get(&realtime, 99) != 0 || errno != EINVAL) {
        return 7;
    }

    errno = EIO;
    broken = gmtime(&epoch);
    if (broken == (struct tm *)0 || broken->tm_year != 70 ||
        broken->tm_mon != 0 || broken->tm_mday != 1 ||
        broken->tm_hour != 0 || broken->tm_min != 0 || broken->tm_sec != 0 ||
        broken->tm_wday != 4 || broken->tm_yday != 0 ||
        broken->tm_isdst != 0 || errno != EIO) {
        return 8;
    }
    if (localtime(&epoch) != broken || errno != EIO) {
        return 9;
    }

    broken = gmtime(&before_epoch);
    if (broken == (struct tm *)0 || broken->tm_year != 69 ||
        broken->tm_mon != 11 || broken->tm_mday != 31 ||
        broken->tm_hour != 23 || broken->tm_min != 59 || broken->tm_sec != 59 ||
        broken->tm_wday != 3 || broken->tm_yday != 364 || errno != EIO) {
        return 10;
    }

    broken = gmtime(&leap);
    if (broken == (struct tm *)0 || broken->tm_year != 100 ||
        broken->tm_mon != 1 || broken->tm_mday != 29 ||
        broken->tm_wday != 2 || broken->tm_yday != 59) {
        return 11;
    }
    leap_tm = *broken;
    if (!same_string(asctime(&leap_tm), "Tue Feb 29 00:00:00 2000\n") ||
        !same_string(ctime(&leap), "Tue Feb 29 00:00:00 2000\n") ||
        errno != EIO) {
        return 12;
    }
    if (strftime(calendar, sizeof(calendar),
                 "%a|%A|%b|%B|%F|%T|%j|%w|%u|%U|%W|%G-W%V-%u|%z|%Z|%c",
                 &leap_tm) != sizeof(expected_calendar) - 1U ||
        !same_string(calendar, expected_calendar) || errno != EIO) {
        return 13;
    }

    broken = gmtime(&nonleap);
    if (broken == (struct tm *)0 || broken->tm_year != 200 ||
        broken->tm_mon != 2 || broken->tm_mday != 1 ||
        broken->tm_yday != 59 || broken->tm_wday != 1) {
        return 14;
    }
    broken = gmtime(&boundary);
    if (broken == (struct tm *)0 || broken->tm_year != 138 ||
        broken->tm_mon != 0 || broken->tm_mday != 19 ||
        broken->tm_hour != 3 || broken->tm_min != 14 || broken->tm_sec != 7 ||
        broken->tm_wday != 2 || broken->tm_yday != 18) {
        return 15;
    }

    normalized.tm_sec = 70;
    normalized.tm_min = 59;
    normalized.tm_hour = 23;
    normalized.tm_mday = 31;
    normalized.tm_mon = 11;
    normalized.tm_year = 99;
    normalized.tm_wday = 0;
    normalized.tm_yday = 0;
    normalized.tm_isdst = 1;
    stamp = mktime(&normalized);
    if (stamp != 946684810L || normalized.tm_year != 100 ||
        normalized.tm_mon != 0 || normalized.tm_mday != 1 ||
        normalized.tm_hour != 0 || normalized.tm_min != 0 ||
        normalized.tm_sec != 10 || normalized.tm_wday != 6 ||
        normalized.tm_yday != 0 || normalized.tm_isdst != 0 || errno != EIO) {
        return 16;
    }

    normalized.tm_sec = 59;
    normalized.tm_min = 59;
    normalized.tm_hour = 23;
    normalized.tm_mday = 31;
    normalized.tm_mon = 11;
    normalized.tm_year = 69;
    normalized.tm_wday = 0;
    normalized.tm_yday = 0;
    normalized.tm_isdst = -1;
    errno = EIO;
    if (mktime(&normalized) != (time_t)-1 || errno != EIO ||
        normalized.tm_year != 69 || normalized.tm_mon != 11 ||
        normalized.tm_mday != 31 || normalized.tm_hour != 23 ||
        normalized.tm_min != 59 || normalized.tm_sec != 59) {
        return 17;
    }

    small[0] = 'Q';
    small[1] = 'Q';
    small[2] = 'Q';
    small[3] = 'Q';
    small[4] = 'Q';
    errno = EIO;
    if (strftime(small, sizeof(small), "%F", &leap_tm) != 0U ||
        small[4] != '\0' || errno != EIO) {
        return 18;
    }
    errno = EIO;
    if (strftime(calendar, sizeof(calendar), "%q", &leap_tm) != 0U ||
        errno != EINVAL) {
        return 19;
    }
    errno = EIO;
    if (gmtime((const time_t *)0) != (struct tm *)0 || errno != EINVAL) {
        return 20;
    }
    errno = EIO;
    if (gmtime(&extreme) != (struct tm *)0 || errno != ERANGE) {
        return 21;
    }

    errno = EIO;
    main_tm_slot = gmtime(&epoch);
    main_text_slot = asctime(main_tm_slot);
    if (main_tm_slot == (struct tm *)0 || main_text_slot == (char *)0 ||
        !same_string(main_text_slot, "Thu Jan  1 00:00:00 1970\n") ||
        thrd_create(&worker, calendar_worker, (void *)0) != thrd_success ||
        thrd_join(worker, &worker_status) != thrd_success || worker_status != 0 ||
        worker_result.tm_slot == main_tm_slot ||
        worker_result.text_slot == main_text_slot ||
        worker_result.year != 100 || worker_result.month != 1 ||
        worker_result.day != 29 || main_tm_slot->tm_year != 70 ||
        main_tm_slot->tm_mon != 0 || main_tm_slot->tm_mday != 1 ||
        !same_string(main_text_slot, "Thu Jan  1 00:00:00 1970\n") ||
        errno != EIO) {
        return 22;
    }

    if (mini_sys_write(1, ok, sizeof(ok) - 1U) !=
        (long)(sizeof(ok) - 1U)) {
        return 23;
    }
    return 0;
}
