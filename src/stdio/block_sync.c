#include <stddef.h>
#include <stdio.h>

#define MINI_STDIO_BYTE_PUBLIC_WRAPPER 1
#include "stdio_internal.h"

size_t __mini_fread_byte_core(void *restrict ptr, size_t size, size_t nmemb,
                              FILE *restrict stream);
size_t __mini_fwrite_byte_core(const void *restrict ptr, size_t size,
                               size_t nmemb, FILE *restrict stream);

size_t fread(void *restrict ptr, size_t size, size_t nmemb,
             FILE *restrict stream)
{
    size_t result;

    __mini_stdio_lock();
    if (__mini_stdio_require_byte(stream) == EOF) {
        result = 0U;
    } else {
        result = __mini_fread_byte_core(ptr, size, nmemb, stream);
    }
    __mini_stdio_unlock();
    return result;
}

size_t fwrite(const void *restrict ptr, size_t size, size_t nmemb,
              FILE *restrict stream)
{
    size_t result;

    __mini_stdio_lock();
    if (__mini_stdio_require_byte(stream) == EOF) {
        result = 0U;
    } else {
        result = __mini_fwrite_byte_core(ptr, size, nmemb, stream);
    }
    __mini_stdio_unlock();
    return result;
}
