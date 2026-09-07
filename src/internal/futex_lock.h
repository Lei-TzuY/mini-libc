#ifndef MINI_LIBC_INTERNAL_FUTEX_LOCK_H
#define MINI_LIBC_INTERNAL_FUTEX_LOCK_H

#include <mini/syscall.h>
#include <stdatomic.h>

#define MINI_PRIVATE_FUTEX_WAIT 0
#define MINI_PRIVATE_FUTEX_WAKE 1

struct mini_futex_lock {
    atomic_int state;
};

#define MINI_FUTEX_LOCK_INIT {ATOMIC_VAR_INIT(0)}

_Static_assert(sizeof(atomic_int) == sizeof(int),
               "private futex lock must preserve the 32-bit futex word");
_Static_assert(ATOMIC_INT_LOCK_FREE == 2,
               "private futex lock requires lock-free atomic_int");

static inline volatile int *mini_futex_lock_word(struct mini_futex_lock *lock)
{
    return (volatile int *)&lock->state;
}

static inline void mini_futex_lock_acquire(struct mini_futex_lock *lock)
{
    while (atomic_exchange(&lock->state, 1) != 0) {
        (void)mini_sys_futex(mini_futex_lock_word(lock), MINI_PRIVATE_FUTEX_WAIT,
                             1, (const void *)0, (volatile int *)0, 0);
    }
}

static inline void mini_futex_lock_release(struct mini_futex_lock *lock)
{
    if (atomic_exchange(&lock->state, 0) != 0) {
        (void)mini_sys_futex(mini_futex_lock_word(lock), MINI_PRIVATE_FUTEX_WAKE,
                             1, (const void *)0, (volatile int *)0, 0);
    }
}

#endif
