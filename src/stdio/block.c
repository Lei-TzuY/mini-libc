#include <errno.h>
#include <stddef.h>
#include <stdio.h>

#include "stdio_internal.h"

static size_t mark_transfer_error(FILE *stream, int error,
                                  size_t completed, size_t size)
{
    if (stream != (FILE *)0) {
        stream->state |= MINI_FILE_ERROR;
    }
    errno = error;
    return completed / size;
}

size_t fread(void *restrict ptr, size_t size, size_t nmemb,
             FILE *restrict stream)
{
    unsigned char *buffer = (unsigned char *)ptr;
    size_t total;
    size_t completed;

    if (size == 0 || nmemb == 0) {
        return 0;
    }
    if (nmemb > (size_t)-1 / size) {
        return mark_transfer_error(stream, EINVAL, 0, size);
    }

    total = size * nmemb;
    completed = __mini_stdio_read(stream, buffer, total);
    return completed / size;
}

size_t fwrite(const void *restrict ptr, size_t size, size_t nmemb,
              FILE *restrict stream)
{
    const unsigned char *buffer = (const unsigned char *)ptr;
    size_t total;
    size_t accepted;

    if (size == 0 || nmemb == 0) {
        return 0;
    }
    if (stream == (FILE *)0 || (stream->mode & MINI_FILE_WRITABLE) == 0U) {
        return mark_transfer_error(stream, EINVAL, 0, size);
    }
    if (nmemb > (size_t)-1 / size) {
        return mark_transfer_error(stream, EINVAL, 0, size);
    }

    total = size * nmemb;
    accepted = __mini_stdio_write(stream, buffer, total);
    return accepted / size;
}
