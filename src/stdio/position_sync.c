#include <stdio.h>

#define MINI_STDIO_SYNC_PUBLIC_WRAPPER 1
#include "stdio_internal.h"

int __mini_fseek_unlocked(FILE *stream, long offset, int whence);
long __mini_ftell_unlocked(FILE *stream);
void __mini_rewind_unlocked(FILE *stream);

int fseek(FILE *stream, long offset, int whence)
{
    int result;

    __mini_stdio_lock();
    result = __mini_fseek_unlocked(stream, offset, whence);
    __mini_stdio_unlock();
    return result;
}

long ftell(FILE *stream)
{
    long result;

    __mini_stdio_lock();
    result = __mini_ftell_unlocked(stream);
    __mini_stdio_unlock();
    return result;
}

void rewind(FILE *stream)
{
    __mini_stdio_lock();
    __mini_rewind_unlocked(stream);
    __mini_stdio_unlock();
}
