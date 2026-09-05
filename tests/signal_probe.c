#include <errno.h>
#include <mini/syscall.h>
#include <signal.h>

static volatile sig_atomic_t seen_signal;

static void record_signal(int sig)
{
    seen_signal = sig;
}

static int emit_ok(void)
{
    static const char text[] = "signal-ok\n";
    return mini_sys_write(1, text, sizeof(text) - 1U) ==
                   (long)(sizeof(text) - 1U)
               ? 0
               : 1;
}

int main(void)
{
    void (*previous)(int);

    errno = 71;
    previous = signal(SIGTERM, record_signal);
    if (previous != SIG_DFL || errno != 71) {
        return 1;
    }

    seen_signal = 0;
    errno = 72;
    if (raise(SIGTERM) != 0 || seen_signal != SIGTERM || errno != 72) {
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

    previous = signal(SIGTERM, record_signal);
    if (previous != SIG_IGN) {
        return 5;
    }
    if (raise(SIGTERM) != 0 || seen_signal != SIGTERM) {
        return 6;
    }

    previous = signal(SIGTERM, SIG_DFL);
    if (previous != record_signal) {
        return 7;
    }

    return emit_ok();
}
