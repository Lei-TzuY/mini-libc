#include <errno.h>
#include <mini/syscall.h>
#include <signal.h>

static volatile sig_atomic_t seen_signal;

static void record_signal(int sig)
{
    seen_signal = sig;
}

int main(void)
{
    static const char text[] = "tiny-signal-ok\n";
    void (*previous)(int);

    errno = 61;
    previous = signal(SIGTERM, record_signal);
    if (previous != SIG_DFL || errno != 61) {
        return 1;
    }

    seen_signal = 0;
    if (raise(SIGTERM) != 0 || seen_signal != SIGTERM) {
        return 2;
    }

    previous = signal(SIGTERM, SIG_IGN);
    if (previous != record_signal) {
        return 3;
    }
    seen_signal = 0;
    if (raise(SIGTERM) != 0 || seen_signal != 0) {
        return 4;
    }

    previous = signal(SIGTERM, SIG_DFL);
    if (previous != SIG_IGN) {
        return 5;
    }

    if (mini_sys_write(1, text, sizeof(text) - 1U) !=
        (long)(sizeof(text) - 1U)) {
        return 6;
    }
    return 0;
}
