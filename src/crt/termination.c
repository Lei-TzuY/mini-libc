#include <mini/syscall.h>
#include <stdlib.h>

#include "../stdio/stdio_internal.h"

#define MINI_ATEXIT_CAPACITY 32U

static void (*mini_atexit_handlers[MINI_ATEXIT_CAPACITY])(void);
static unsigned int mini_atexit_count;

int atexit(void (*func)(void))
{
    if (func == (void (*)(void))0 || mini_atexit_count == MINI_ATEXIT_CAPACITY) {
        return -1;
    }

    mini_atexit_handlers[mini_atexit_count] = func;
    ++mini_atexit_count;
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
