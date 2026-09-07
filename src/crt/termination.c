#include <mini/syscall.h>
#include <stdlib.h>

#include "../internal/futex_lock.h"

#define MINI_ATEXIT_CAPACITY 32U
#define MINI_QUICK_EXIT_CAPACITY 32U

int __mini_stdio_flush_all(void);

static void (*mini_atexit_handlers[MINI_ATEXIT_CAPACITY])(void);
static unsigned int mini_atexit_count;
static struct mini_futex_lock mini_atexit_lock = MINI_FUTEX_LOCK_INIT;
static void (*mini_quick_exit_handlers[MINI_QUICK_EXIT_CAPACITY])(void);
static unsigned int mini_quick_exit_count;
static struct mini_futex_lock mini_quick_exit_lock = MINI_FUTEX_LOCK_INIT;

int atexit(void (*func)(void))
{
    int result = -1;

    if (func == (void (*)(void))0) {
        return -1;
    }

    mini_futex_lock_acquire(&mini_atexit_lock);
    if (mini_atexit_count != MINI_ATEXIT_CAPACITY) {
        mini_atexit_handlers[mini_atexit_count] = func;
        ++mini_atexit_count;
        result = 0;
    }
    mini_futex_lock_release(&mini_atexit_lock);
    return result;
}

int at_quick_exit(void (*func)(void))
{
    int result = -1;

    if (func == (void (*)(void))0) {
        return -1;
    }

    mini_futex_lock_acquire(&mini_quick_exit_lock);
    if (mini_quick_exit_count != MINI_QUICK_EXIT_CAPACITY) {
        mini_quick_exit_handlers[mini_quick_exit_count] = func;
        ++mini_quick_exit_count;
        result = 0;
    }
    mini_futex_lock_release(&mini_quick_exit_lock);
    return result;
}

_Noreturn void _Exit(int status)
{
    mini_sys_exit_group(status);
}

_Noreturn void exit(int status)
{
    for (;;) {
        void (*handler)(void);

        mini_futex_lock_acquire(&mini_atexit_lock);
        if (mini_atexit_count == 0U) {
            mini_futex_lock_release(&mini_atexit_lock);
            break;
        }
        --mini_atexit_count;
        handler = mini_atexit_handlers[mini_atexit_count];
        mini_atexit_handlers[mini_atexit_count] = (void (*)(void))0;
        mini_futex_lock_release(&mini_atexit_lock);

        handler();
    }

    (void)__mini_stdio_flush_all();
    _Exit(status);
}

_Noreturn void quick_exit(int status)
{
    for (;;) {
        void (*handler)(void);

        mini_futex_lock_acquire(&mini_quick_exit_lock);
        if (mini_quick_exit_count == 0U) {
            mini_futex_lock_release(&mini_quick_exit_lock);
            break;
        }
        --mini_quick_exit_count;
        handler = mini_quick_exit_handlers[mini_quick_exit_count];
        mini_quick_exit_handlers[mini_quick_exit_count] = (void (*)(void))0;
        mini_futex_lock_release(&mini_quick_exit_lock);

        handler();
    }

    _Exit(status);
}
