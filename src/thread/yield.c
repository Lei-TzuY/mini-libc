#include <errno.h>
#include <mini/syscall.h>
#include <threads.h>

void thrd_yield(void)
{
    int saved_errno = errno;

    (void)mini_sys_sched_yield();
    errno = saved_errno;
}
