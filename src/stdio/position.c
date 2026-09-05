#include <errno.h>
#include <mini/syscall.h>
#include <stdio.h>

#include "stdio_internal.h"

static int valid_stream(FILE *stream)
{
    return stream != (FILE *)0 &&
           (stream->mode & (MINI_FILE_READABLE | MINI_FILE_WRITABLE)) != 0U;
}

int fseek(FILE *stream, long offset, int whence)
{
    long result;

    if (!valid_stream(stream) ||
        (whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END)) {
        errno = EINVAL;
        return -1;
    }
    if ((stream->mode & MINI_FILE_WRITABLE) != 0U &&
        __mini_stdio_flush_buffer(stream) == EOF) {
        return -1;
    }

    result = mini_sys_lseek(stream->fd, offset, whence);
    if (result < 0) {
        errno = (int)-result;
        return -1;
    }

    stream->state &= ~(MINI_FILE_EOF | MINI_FILE_READ_NEEDS_POSITION |
                       MINI_FILE_WRITE_NEEDS_SYNC);
    return 0;
}

long ftell(FILE *stream)
{
    unsigned long max_long = (~0UL) >> 1;
    long result;

    if (!valid_stream(stream)) {
        errno = EINVAL;
        return -1L;
    }

    result = mini_sys_lseek(stream->fd, 0L, SEEK_CUR);
    if (result < 0) {
        errno = (int)-result;
        return -1L;
    }
    if ((unsigned long)result > max_long - (unsigned long)stream->write_length) {
        errno = EINVAL;
        return -1L;
    }
    return result + (long)stream->write_length;
}

void rewind(FILE *stream)
{
    (void)fseek(stream, 0L, SEEK_SET);
    if (stream != (FILE *)0) {
        stream->state &= ~(MINI_FILE_EOF | MINI_FILE_ERROR);
    }
}
