#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>

static volatile sig_atomic_t seen_signal;

static void record_signal(int sig)
{
    seen_signal = sig;
}

int main(void)
{
    struct timespec now;
    time_t wall;
    time_t stored;
    clock_t before;
    clock_t after;
    double delta;
    volatile unsigned long work = 1UL;
    unsigned long i;
    void (*previous)(int);

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
