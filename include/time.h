#ifndef MINI_LIBC_TIME_H
#define MINI_LIBC_TIME_H

typedef long clock_t;
typedef long time_t;

struct timespec {
    time_t tv_sec;
    long tv_nsec;
};

#define CLOCKS_PER_SEC 1000000L
#define TIME_UTC 1

clock_t clock(void);
double difftime(time_t time1, time_t time0);
time_t time(time_t *timer);
int timespec_get(struct timespec *ts, int base);

#endif
