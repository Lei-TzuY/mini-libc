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

    result = mini_sys_lseek(stream->fd, offset, whence);
    if (result < 0) {
        errno = (int)-result;
        return -1;
    }

    stream->state &= ~MINI_FILE_EOF;
    return 0;
}

long ftell(FILE *stream)
{
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
    return result;
}

void rewind(FILE *stream)
{
    if (!valid_stream(stream)) {
        errno = EINVAL;
        return;
    }

    stream->state &= ~(MINI_FILE_EOF | MINI_FILE_ERROR);
    if (mini_sys_lseek(stream->fd, 0L, SEEK_SET) < 0) {
        long result = mini_sys_lseek(stream->fd, 0L, SEEK_CUR);

        if (result < 0) {
            errno = (int)-result;
        }
    }
}
