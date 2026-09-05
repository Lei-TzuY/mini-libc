#include <mini/syscall.h>

#include "../internal/thread_runtime.h"

#define MINI_ARCH_SET_FS 0x1002
#define MINI_THREAD_INIT_FAILURE 127

static struct mini_thread_tcb mini_main_tcb;

static int *thread_errno_location(void)
{
    struct mini_thread_tcb *tcb = __mini_thread_current_tcb();

    return &tcb->errno_value;
}

void __mini_thread_runtime_init_main(void)
{
    long result;

    mini_main_tcb.self = &mini_main_tcb;
    mini_main_tcb.control = (void *)0;
    mini_main_tcb.errno_value = 0;
    mini_main_tcb.reserved = 0U;

    result = mini_sys_arch_prctl(MINI_ARCH_SET_FS,
                                 (unsigned long)&mini_main_tcb);
    if (result < 0) {
        mini_sys_exit(MINI_THREAD_INIT_FAILURE);
    }

    __mini_errno_set_provider(thread_errno_location);
}
