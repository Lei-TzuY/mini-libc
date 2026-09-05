#include <mini/syscall.h>
#include <stdlib.h>

#define MINI_ATEXIT_CAPACITY 32U
#define MINI_QUICK_EXIT_CAPACITY 32U

int __mini_stdio_flush_all(void);

static void (*mini_atexit_handlers[MINI_ATEXIT_CAPACITY])(void);
static unsigned int mini_atexit_count;
static void (*mini_quick_exit_handlers[MINI_QUICK_EXIT_CAPACITY])(void);
static unsigned int mini_quick_exit_count;

int atexit(void (*func)(void))
{
    if (func == (void (*)(void))0 || mini_atexit_count == MINI_ATEXIT_CAPACITY) {
        return -1;
    }

    mini_atexit_handlers[mini_atexit_count] = func;
    ++mini_atexit_count;
    return 0;
}

int at_quick_exit(void (*func)(void))
{
    if (func == (void (*)(void))0 ||
        mini_quick_exit_count == MINI_QUICK_EXIT_CAPACITY) {
        return -1;
    }

    mini_quick_exit_handlers[mini_quick_exit_count] = func;
    ++mini_quick_exit_count;
    return 0;
}

_Noreturn void _Exit(int status)
{
    mini_sys_exit(status);
}

_Noreturn void exit(int status)
{
    while (mini_atexit_count != 0U) {
        void (*handler)(void);

        --mini_atexit_count;
        handler = mini_atexit_handlers[mini_atexit_count];
        mini_atexit_handlers[mini_atexit_count] = (void (*)(void))0;
        handler();
    }

    (void)__mini_stdio_flush_all();
    _Exit(status);
}

_Noreturn void quick_exit(int status)
{
    while (mini_quick_exit_count != 0U) {
        void (*handler)(void);

        --mini_quick_exit_count;
        handler = mini_quick_exit_handlers[mini_quick_exit_count];
        mini_quick_exit_handlers[mini_quick_exit_count] = (void (*)(void))0;
        handler();
    }

    _Exit(status);
}
