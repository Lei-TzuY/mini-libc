#include <errno.h>
#include <mini/syscall.h>
#include <time.h>

int main(void)
{
    static const char ok[] = "time-ok\n";
    struct timespec realtime;
    time_t wall;
    time_t stored;
    clock_t before;
    clock_t after;
    double delta;
    volatile unsigned long work = 0UL;
    unsigned long i;

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
    if (work == 0UL && after < before) {
        return 6;
    }

    errno = EIO;
    if (timespec_get(&realtime, 99) != 0 || errno != EINVAL) {
        return 7;
    }

    if (mini_sys_write(1, ok, sizeof(ok) - 1U) !=
        (long)(sizeof(ok) - 1U)) {
        return 8;
    }
    return 0;
}
