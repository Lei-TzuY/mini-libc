#include <stdio.h>

#define MINI_STDIO_SYNC_PUBLIC_WRAPPER 1
#define MINI_STDIO_BYTE_PUBLIC_WRAPPER 1
#include "stdio_internal.h"

struct mini_format_args;

int __mini_format_dispatch_unlocked(FILE *stream, const char *format,
                                    struct mini_format_args *args);

int __mini_format_dispatch(FILE *stream, const char *format,
                           struct mini_format_args *args)
{
    int result;

    __mini_stdio_lock();
    if (__mini_stdio_require_byte(stream) == EOF) {
        result = EOF;
    } else {
        result = __mini_format_dispatch_unlocked(stream, format, args);
    }
    __mini_stdio_unlock();
    return result;
}
