#include <errno.h>
#include <mini/syscall.h>
#include <signal.h>

#define MINI_SA_RESTORER 0x04000000UL
#define MINI_KERNEL_SIGSET_SIZE 8UL

struct mini_kernel_sigaction {
    void (*handler)(int);
    unsigned long flags;
    void (*restorer)(void);
    unsigned long mask;
};

extern void (*__mini_signal_restorer(void))(void);

_Static_assert(sizeof(struct mini_kernel_sigaction) == 32U,
               "x86-64 kernel sigaction layout must remain 32 bytes");

void (*signal(int sig, void (*func)(int)))(int)
{
    struct mini_kernel_sigaction action;
    struct mini_kernel_sigaction previous;
    int saved_errno = errno;
    long result;

    action.handler = func;
    action.flags = MINI_SA_RESTORER;
    action.restorer = __mini_signal_restorer();
    action.mask = 0UL;

    result = mini_sys_rt_sigaction(sig, &action, &previous,
                                   MINI_KERNEL_SIGSET_SIZE);
    if (result < 0) {
        errno = (int)-result;
        return SIG_ERR;
    }

    errno = saved_errno;
    return previous.handler;
}

int raise(int sig)
{
    int saved_errno = errno;
    long pid = mini_sys_getpid();
    long tid = mini_sys_gettid();
    long result = mini_sys_tgkill((int)pid, (int)tid, sig);

    if (result < 0) {
        errno = (int)-result;
        return -1;
    }

    errno = saved_errno;
    return 0;
}
