#include <errno.h>
#include <mini/syscall.h>
#include <stdio.h>

#include "stdio_internal.h"

static int valid_stream(FILE *stream)
{
    return stream != (FILE *)0 &&
           (stream->mode & (MINI_FILE_READABLE | MINI_FILE_WRITABLE)) != 0U;
}

static size_t unread_input(FILE *stream)
{
    size_t unread = 0;

    if (stream->read_length >= stream->read_offset) {
        unread = stream->read_length - stream->read_offset;
    }
    if (stream->pushback_valid != 0U) {
        ++unread;
    }
    return unread;
}

static void discard_input(FILE *stream)
{
    stream->read_offset = 0;
    stream->read_length = 0;
    stream->pushback_valid = 0U;
    stream->pushback_byte = 0U;
}

int fseek(FILE *stream, long offset, int whence)
{
    unsigned long max_long = (~0UL) >> 1;
    long min_long = -(long)max_long - 1L;
    long adjusted = offset;
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

    if (whence == SEEK_CUR) {
        size_t unread = unread_input(stream);

        if (unread > (size_t)max_long ||
            offset < min_long + (long)unread) {
            errno = EINVAL;
            return -1;
        }
        adjusted = offset - (long)unread;
    }

    result = mini_sys_lseek(stream->fd, adjusted, whence);
    if (result < 0) {
        errno = (int)-result;
        return -1;
    }

    discard_input(stream);
    stream->state &= ~(MINI_FILE_EOF | MINI_FILE_READ_NEEDS_POSITION |
                       MINI_FILE_WRITE_NEEDS_SYNC);
    return 0;
}

long ftell(FILE *stream)
{
    unsigned long max_long = (~0UL) >> 1;
    size_t unread;
    long result;
    long logical;

    if (!valid_stream(stream)) {
        errno = EINVAL;
        return -1L;
    }

    if ((stream->mode & MINI_FILE_APPEND) != 0U && stream->write_length != 0U &&
        __mini_stdio_flush_buffer(stream) == EOF) {
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

    logical = result + (long)stream->write_length;
    unread = unread_input(stream);
    if (unread > (size_t)logical) {
        errno = EINVAL;
        return -1L;
    }
    return logical - (long)unread;
}

void rewind(FILE *stream)
{
    (void)fseek(stream, 0L, SEEK_SET);
    if (stream != (FILE *)0) {
        stream->state &= ~(MINI_FILE_EOF | MINI_FILE_ERROR);
    }
}
