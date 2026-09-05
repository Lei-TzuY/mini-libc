#include <errno.h>
#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>

struct captured_kernel_sigaction {
    void (*handler)(int);
    unsigned long flags;
    void (*restorer)(void);
    unsigned long mask;
};

static long rt_sigaction_result;
static void (*reported_old_handler)(int);
static int captured_signal;
static unsigned long captured_sigset_size;
static struct captured_kernel_sigaction captured_action;
static int rt_sigaction_calls;

static long fake_pid = 1234;
static long fake_tid = 5678;
static long tgkill_result;
static int captured_tgid;
static int captured_tid;
static int captured_tgkill_signal;
static int getpid_calls;
static int gettid_calls;
static int tgkill_calls;

static jmp_buf abort_jump;
static int abort_capture;
static int abort_events[4];
static unsigned int abort_event_count;
static int abort_fallback_status;

static void record_abort_event(int event)
{
    if (abort_capture && abort_event_count < 4U) {
        abort_events[abort_event_count] = event;
        ++abort_event_count;
    }
}

static void handler_a(int sig)
{
    (void)sig;
}

static void handler_b(int sig)
{
    (void)sig;
}

void __mini_rt_sigreturn_restorer(void)
{
}

void (*__mini_signal_restorer(void))(void)
{
    return __mini_rt_sigreturn_restorer;
}

long mini_test_rt_sigaction(int sig, const void *act, void *oldact,
                            unsigned long sigsetsize)
{
    const struct captured_kernel_sigaction *input =
        (const struct captured_kernel_sigaction *)act;
    struct captured_kernel_sigaction *output =
        (struct captured_kernel_sigaction *)oldact;

    record_abort_event(2);
    ++rt_sigaction_calls;
    captured_signal = sig;
    captured_sigset_size = sigsetsize;
    captured_action = *input;

    if (rt_sigaction_result < 0) {
        return rt_sigaction_result;
    }

    output->handler = reported_old_handler;
    output->flags = 0UL;
    output->restorer = (void (*)(void))0;
    output->mask = 0UL;
    return 0;
}

long mini_test_getpid(void)
{
    ++getpid_calls;
    return fake_pid;
}

long mini_test_gettid(void)
{
    ++gettid_calls;
    return fake_tid;
}

long mini_test_tgkill(int tgid, int tid, int sig)
{
    record_abort_event(1);
    ++tgkill_calls;
    captured_tgid = tgid;
    captured_tid = tid;
    captured_tgkill_signal = sig;
    return tgkill_result;
}

_Noreturn void mini_test__Exit(int status)
{
    record_abort_event(3);
    abort_fallback_status = status;
    longjmp(abort_jump, 1);
}

static void reset_action_state(void)
{
    rt_sigaction_result = 0;
    reported_old_handler = SIG_DFL;
    captured_signal = 0;
    captured_sigset_size = 0UL;
    captured_action.handler = SIG_DFL;
    captured_action.flags = 0UL;
    captured_action.restorer = (void (*)(void))0;
    captured_action.mask = ~0UL;
    rt_sigaction_calls = 0;
}

static void reset_raise_state(void)
{
    fake_pid = 1234;
    fake_tid = 5678;
    tgkill_result = 0;
    captured_tgid = 0;
    captured_tid = 0;
    captured_tgkill_signal = 0;
    getpid_calls = 0;
    gettid_calls = 0;
    tgkill_calls = 0;
}

static void reset_abort_state(void)
{
    unsigned int i;

    abort_capture = 0;
    abort_event_count = 0U;
    abort_fallback_status = -1;
    for (i = 0; i < 4U; ++i) {
        abort_events[i] = 0;
    }
}

int main(void)
{
    void (*previous)(int);

    reset_action_state();
    reported_old_handler = handler_b;
    errno = 91;
    previous = signal(SIGTERM, handler_a);
    if (previous != handler_b || errno != 91 || rt_sigaction_calls != 1 ||
        captured_signal != SIGTERM || captured_sigset_size != 8UL ||
        captured_action.handler != handler_a ||
        captured_action.flags != 0x04000000UL ||
        captured_action.restorer != __mini_rt_sigreturn_restorer ||
        captured_action.mask != 0UL) {
        return 1;
    }

    reset_action_state();
    reported_old_handler = handler_a;
    previous = signal(SIGTERM, SIG_IGN);
    if (previous != handler_a || captured_action.handler != SIG_IGN) {
        return 2;
    }

    reset_action_state();
    rt_sigaction_result = -22;
    errno = 0;
    previous = signal(SIGTERM, handler_a);
    if (previous != SIG_ERR || errno != EINVAL || rt_sigaction_calls != 1) {
        return 3;
    }

    reset_raise_state();
    errno = 92;
    if (raise(SIGINT) != 0 || errno != 92 || getpid_calls != 1 ||
        gettid_calls != 1 || tgkill_calls != 1 || captured_tgid != 1234 ||
        captured_tid != 5678 || captured_tgkill_signal != SIGINT) {
        return 4;
    }

    reset_raise_state();
    tgkill_result = -5;
    errno = 0;
    if (raise(SIGABRT) != -1 || errno != EIO || getpid_calls != 1 ||
        gettid_calls != 1 || tgkill_calls != 1 ||
        captured_tgkill_signal != SIGABRT) {
        return 5;
    }

    reset_action_state();
    reset_raise_state();
    reset_abort_state();
    reported_old_handler = handler_a;
    abort_capture = 1;
    if (setjmp(abort_jump) == 0) {
        abort();
    }
    abort_capture = 0;
    if (abort_event_count != 4U || abort_events[0] != 1 ||
        abort_events[1] != 2 || abort_events[2] != 1 ||
        abort_events[3] != 3 || rt_sigaction_calls != 1 ||
        captured_signal != SIGABRT || captured_action.handler != SIG_DFL ||
        tgkill_calls != 2 || getpid_calls != 2 || gettid_calls != 2 ||
        captured_tgkill_signal != SIGABRT || abort_fallback_status != 134) {
        return 6;
    }

    return 0;
}
