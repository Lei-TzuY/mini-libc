#include <stddef.h>
#include <stdio.h>

#include "stdio_internal.h"

int __mini_setvbuf_unlocked(FILE *restrict stream, char *restrict buf,
                            int mode, size_t size);
void __mini_setbuf_unlocked(FILE *restrict stream, char *restrict buf);
FILE *__mini_freopen_unlocked(const char *restrict filename,
                              const char *restrict mode,
                              FILE *restrict stream);
int __mini_fclose_unlocked(FILE *stream);

int setvbuf(FILE *restrict stream, char *restrict buf, int mode, size_t size)
{
    int result;

    __mini_stdio_lock();
    result = __mini_setvbuf_unlocked(stream, buf, mode, size);
    __mini_stdio_unlock();
    return result;
}

void setbuf(FILE *restrict stream, char *restrict buf)
{
    __mini_stdio_lock();
    __mini_setbuf_unlocked(stream, buf);
    __mini_stdio_unlock();
}

FILE *freopen(const char *restrict filename, const char *restrict mode,
              FILE *restrict stream)
{
    FILE *result;

    __mini_stdio_lock();
    result = __mini_freopen_unlocked(filename, mode, stream);
    __mini_stdio_unlock();
    return result;
}

int fclose(FILE *stream)
{
    int result;

    __mini_stdio_lock();
    result = __mini_fclose_unlocked(stream);
    __mini_stdio_unlock();
    return result;
}
