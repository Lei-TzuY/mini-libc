#include <mini/syscall.h>
#include <threads.h>

#define MINI_FUTEX_WAIT 0
#define MINI_FUTEX_WAKE 1
#define MINI_FUTEX_WAKE_ALL 2147483647

#define MINI_ONCE_UNINITIALIZED 0
#define MINI_ONCE_RUNNING 1
#define MINI_ONCE_COMPLETE 2

static volatile int mini_once_lock_word;

extern int __mini_atomic_exchange_int(volatile int *value, int replacement);

static void once_lock(void)
{
    while (__mini_atomic_exchange_int(&mini_once_lock_word, 1) != 0) {
        (void)mini_sys_futex(&mini_once_lock_word, MINI_FUTEX_WAIT, 1,
                             (const void *)0, (volatile int *)0, 0);
    }
}

static void once_unlock(void)
{
    if (__mini_atomic_exchange_int(&mini_once_lock_word, 0) != 0) {
        (void)mini_sys_futex(&mini_once_lock_word, MINI_FUTEX_WAKE, 1,
                             (const void *)0, (volatile int *)0, 0);
    }
}

void call_once(once_flag *flag, void (*func)(void))
{
    for (;;) {
        int state;

        once_lock();
        state = flag->__state;
        if (state == MINI_ONCE_COMPLETE) {
            once_unlock();
            return;
        }
        if (state == MINI_ONCE_UNINITIALIZED) {
            flag->__state = MINI_ONCE_RUNNING;
            once_unlock();

            func();

            once_lock();
            flag->__state = MINI_ONCE_COMPLETE;
            once_unlock();
            (void)mini_sys_futex((volatile int *)&flag->__state,
                                 MINI_FUTEX_WAKE, MINI_FUTEX_WAKE_ALL,
                                 (const void *)0, (volatile int *)0, 0);
            return;
        }
        once_unlock();

        (void)mini_sys_futex((volatile int *)&flag->__state, MINI_FUTEX_WAIT,
                             MINI_ONCE_RUNNING, (const void *)0,
                             (volatile int *)0, 0);
    }
}
