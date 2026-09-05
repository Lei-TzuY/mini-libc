#include <stdio.h>

#include "stdio_internal.h"

struct mini_scan_args;

int __mini_scan_dispatch_unlocked(FILE *stream, const char *format,
                                  struct mini_scan_args *args);

int __mini_scan_dispatch(FILE *stream, const char *format,
                         struct mini_scan_args *args)
{
    int result;

    __mini_stdio_lock();
    result = __mini_scan_dispatch_unlocked(stream, format, args);
    __mini_stdio_unlock();
    return result;
}
