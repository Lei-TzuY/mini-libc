#ifndef MINI_LIBC_TIME_H
#define MINI_LIBC_TIME_H

#include <stddef.h>

typedef long clock_t;
typedef long time_t;

struct timespec {
    time_t tv_sec;
    long tv_nsec;
};

struct tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

#define CLOCKS_PER_SEC 1000000L
#define TIME_UTC 1

clock_t clock(void);
double difftime(time_t time1, time_t time0);
time_t time(time_t *timer);
int timespec_get(struct timespec *ts, int base);

char *asctime(const struct tm *timeptr);
char *ctime(const time_t *timer);
struct tm *gmtime(const time_t *timer);
struct tm *localtime(const time_t *timer);
time_t mktime(struct tm *timeptr);
size_t strftime(char *restrict s, size_t maxsize,
                const char *restrict format,
                const struct tm *restrict timeptr);

#endif
