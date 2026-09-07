#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <threads.h>
#include <time.h>

static volatile sig_atomic_t seen_signal;

struct calendar_thread_result {
    struct tm *tm_slot;
    char *text_slot;
    int ok;
};

static struct calendar_thread_result calendar_result;

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

static void record_signal(int sig)
{
    seen_signal = sig;
}

static int calendar_worker(void *arg)
{
    time_t leap = 951782400L;
    struct tm *broken;
    char *text;

    (void)arg;
    broken = gmtime(&leap);
    text = broken == (struct tm *)0 ? (char *)0 : asctime(broken);
    if (broken == (struct tm *)0 || text == (char *)0 ||
        broken->tm_year != 100 || broken->tm_mon != 1 ||
        broken->tm_mday != 29 ||
        !same_string(text, "Tue Feb 29 00:00:00 2000\n")) {
        return 1;
    }
    calendar_result.tm_slot = broken;
    calendar_result.text_slot = text;
    calendar_result.ok = 1;
    return 0;
}

int main(void)
{
    static const char expected_calendar[] =
        "2000-02-29 00:00:00 2000-W09-2 UTC";
    struct timespec now;
    struct tm calendar_value;
    struct tm normalized;
    struct tm *main_tm_slot;
    char *main_text_slot;
    char calendar_text[64];
    time_t wall;
    time_t stored;
    time_t leap = 951782400L;
    time_t epoch = 0L;
    clock_t before;
    clock_t after;
    double delta;
    volatile unsigned long work = 1UL;
    unsigned long i;
    void (*previous)(int);
    thrd_t worker;
    int worker_status;

    errno = EIO;
    if (timespec_get(&now, TIME_UTC) != TIME_UTC || now.tv_sec <= 0 ||
        now.tv_nsec < 0 || now.tv_nsec >= 1000000000L || errno != EIO) {
        return 1;
    }

    stored = 0;
    wall = time(&stored);
    if (wall == (time_t)-1 || stored != wall || errno != EIO) {
        return 2;
    }
    delta = difftime(wall, now.tv_sec);
    if (delta < -2.0 || delta > 2.0) {
        return 3;
    }

    before = clock();
    if (before == (clock_t)-1 || before < 0 || errno != EIO) {
        return 4;
    }
    for (i = 0UL; i < 100000UL; ++i) {
        work = work * 33UL + i;
    }
    after = clock();
    if (after == (clock_t)-1 || after < before || errno != EIO) {
        return 5;
    }
    (void)work;

    main_tm_slot = gmtime(&leap);
    if (main_tm_slot == (struct tm *)0 || main_tm_slot->tm_year != 100 ||
        main_tm_slot->tm_mon != 1 || main_tm_slot->tm_mday != 29 ||
        main_tm_slot->tm_wday != 2 || main_tm_slot->tm_yday != 59 ||
        errno != EIO) {
        return 12;
    }
    calendar_value = *main_tm_slot;
    if (strftime(calendar_text, sizeof(calendar_text), "%F %T %G-W%V-%u %Z",
                 &calendar_value) != sizeof(expected_calendar) - 1U ||
        !same_string(calendar_text, expected_calendar) || errno != EIO) {
        return 13;
    }
    if (!same_string(asctime(&calendar_value), "Tue Feb 29 00:00:00 2000\n") ||
        !same_string(ctime(&leap), "Tue Feb 29 00:00:00 2000\n") ||
        errno != EIO) {
        return 14;
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
    if (mktime(&normalized) != 946684810L || normalized.tm_year != 100 ||
        normalized.tm_mon != 0 || normalized.tm_mday != 1 ||
        normalized.tm_hour != 0 || normalized.tm_min != 0 ||
        normalized.tm_sec != 10 || normalized.tm_isdst != 0 || errno != EIO) {
        return 15;
    }

    main_tm_slot = gmtime(&epoch);
    main_text_slot = main_tm_slot == (struct tm *)0 ? (char *)0 :
                     asctime(main_tm_slot);
    if (main_tm_slot == (struct tm *)0 || main_text_slot == (char *)0 ||
        !same_string(main_text_slot, "Thu Jan  1 00:00:00 1970\n") ||
        thrd_create(&worker, calendar_worker, (void *)0) != thrd_success ||
        thrd_join(worker, &worker_status) != thrd_success || worker_status != 0 ||
        !calendar_result.ok || calendar_result.tm_slot == main_tm_slot ||
        calendar_result.text_slot == main_text_slot ||
        main_tm_slot->tm_year != 70 ||
        !same_string(main_text_slot, "Thu Jan  1 00:00:00 1970\n") || errno != EIO) {
        return 16;
    }

    previous = signal(SIGTERM, record_signal);
    if (previous != SIG_DFL || errno != EIO) {
        return 6;
    }
    seen_signal = 0;
    if (raise(SIGTERM) != 0 || seen_signal != SIGTERM || errno != EIO) {
        return 7;
    }

    previous = signal(SIGTERM, SIG_IGN);
    if (previous != record_signal) {
        return 8;
    }
    seen_signal = 0;
    if (raise(SIGTERM) != 0 || seen_signal != 0) {
        return 9;
    }

    previous = signal(SIGTERM, SIG_DFL);
    if (previous != SIG_IGN) {
        return 10;
    }

    if (puts("tiny-time-ok") < 0) {
        return 11;
    }
    return 0;
}
