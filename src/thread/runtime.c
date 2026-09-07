#include <mini/syscall.h>

#include "../internal/thread_runtime.h"

#define MINI_ARCH_SET_FS 0x1002
#define MINI_THREAD_INIT_FAILURE 127

struct mini_main_thread_state {
    _Alignas(MINI_COMPILER_TLS_ALIGNMENT)
        unsigned char compiler_tls[MINI_COMPILER_TLS_CAPACITY];
    struct mini_thread_tcb tcb;
};

static struct mini_main_thread_state mini_main_state;

static int *thread_errno_location(void)
{
    struct mini_thread_tcb *tcb = __mini_thread_current_tcb();

    return &tcb->errno_value;
}

void __mini_thread_runtime_init_main(long *initial_stack)
{
    struct mini_thread_tcb *tcb = &mini_main_state.tcb;
    long result;

    if (!__mini_thread_tls_discover(initial_stack)) {
        mini_sys_exit(MINI_THREAD_INIT_FAILURE);
    }

    tcb->self = tcb;
    tcb->control = (void *)0;
    tcb->errno_value = 0;
    tcb->reserved = 0U;
    if (!__mini_thread_tls_prepare(tcb)) {
        mini_sys_exit(MINI_THREAD_INIT_FAILURE);
    }

    result = mini_sys_arch_prctl(MINI_ARCH_SET_FS, (unsigned long)tcb);
    if (result < 0) {
        mini_sys_exit(MINI_THREAD_INIT_FAILURE);
    }

    __mini_errno_set_provider(thread_errno_location);
}
