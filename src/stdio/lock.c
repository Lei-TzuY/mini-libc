#include "../internal/futex_lock.h"
#include "../internal/thread_runtime.h"

static struct mini_futex_lock mini_stdio_lock_word = MINI_FUTEX_LOCK_INIT;

void __mini_stdio_lock(void)
{
    struct mini_thread_tcb *tcb = __mini_thread_current_tcb();

    if (tcb->stdio_lock_depth != 0U) {
        ++tcb->stdio_lock_depth;
        return;
    }

    mini_futex_lock_acquire(&mini_stdio_lock_word);
    tcb->stdio_lock_depth = 1U;
}

void __mini_stdio_unlock(void)
{
    struct mini_thread_tcb *tcb = __mini_thread_current_tcb();

    if (tcb->stdio_lock_depth > 1U) {
        --tcb->stdio_lock_depth;
        return;
    }

    tcb->stdio_lock_depth = 0U;
    mini_futex_lock_release(&mini_stdio_lock_word);
}
