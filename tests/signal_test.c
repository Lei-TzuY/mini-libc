#include <errno.h>
#include <signal.h>

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

long mini_test_rt_sigaction(int sig, const void *act, void *oldact,
                            unsigned long sigsetsize)
{
    const struct captured_kernel_sigaction *input =
        (const struct captured_kernel_sigaction *)act;
    struct captured_kernel_sigaction *output =
        (struct captured_kernel_sigaction *)oldact;

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
    ++tgkill_calls;
    captured_tgid = tgid;
    captured_tid = tid;
    captured_tgkill_signal = sig;
    return tgkill_result;
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

    return 0;
}
