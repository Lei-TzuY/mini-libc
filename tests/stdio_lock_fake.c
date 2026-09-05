#include <stdlib.h>

static int lock_depth;
static int registered;

static void verify_balanced(void)
{
    if (lock_depth != 0) {
        _Exit(122);
    }
}

void __mini_stdio_lock(void)
{
    if (!registered) {
        if (atexit(verify_balanced) != 0) {
            _Exit(123);
        }
        registered = 1;
    }
    ++lock_depth;
}

void __mini_stdio_unlock(void)
{
    if (lock_depth <= 0) {
        _Exit(121);
    }
    --lock_depth;
}
