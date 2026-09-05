#include <errno.h>
#include <mini/syscall.h>
#include <stddef.h>
#include <stdio.h>

#include "stdio_internal.h"

static int readable_stream(FILE *stream)
{
    return stream != (FILE *)0 && (stream->mode & MINI_FILE_READABLE) != 0U;
}

static size_t mark_read_error(FILE *stream, int error, size_t completed)
{
    if (stream != (FILE *)0) {
        stream->state |= MINI_FILE_ERROR;
    }
    errno = error;
    return completed;
}

size_t __mini_stdio_read(FILE *stream, unsigned char *buffer, size_t length)
{
    size_t completed = 0;

    if (length == 0) {
        return 0;
    }
    if (!readable_stream(stream)) {
        return mark_read_error(stream, EINVAL, 0);
    }
    if ((stream->state & MINI_FILE_WRITE_NEEDS_SYNC) != 0U) {
        return mark_read_error(stream, EINVAL, 0);
    }

    while (completed < length) {
        if (stream->pushback_valid != 0U) {
            buffer[completed++] = stream->pushback_byte;
            stream->pushback_valid = 0U;
            stream->state |= MINI_FILE_READ_NEEDS_POSITION;
            continue;
        }

        if (stream->read_offset < stream->read_length) {
            size_t available = stream->read_length - stream->read_offset;
            size_t wanted = length - completed;
            size_t chunk = available < wanted ? available : wanted;
            size_t i;

            for (i = 0; i < chunk; ++i) {
                buffer[completed + i] = stream->read_buffer[stream->read_offset + i];
            }
            stream->read_offset += chunk;
            completed += chunk;
            stream->state |= MINI_FILE_READ_NEEDS_POSITION;
            continue;
        }

        if ((stream->state & MINI_FILE_EOF) != 0U) {
            return completed;
        }

        {
            long result = mini_sys_read(stream->fd, stream->read_buffer,
                                        MINI_FILE_BUFFER_SIZE);

            if (result < 0) {
                stream->state |= MINI_FILE_READ_NEEDS_POSITION;
                return mark_read_error(stream, (int)-result, completed);
            }
            if (result == 0) {
                stream->read_offset = 0;
                stream->read_length = 0;
                stream->state |= MINI_FILE_EOF;
                stream->state &= ~MINI_FILE_READ_NEEDS_POSITION;
                return completed;
            }
            if ((unsigned long)result > (unsigned long)MINI_FILE_BUFFER_SIZE) {
                stream->state |= MINI_FILE_READ_NEEDS_POSITION;
                return mark_read_error(stream, EIO, completed);
            }

            stream->read_offset = 0;
            stream->read_length = (size_t)result;
        }
    }

    return completed;
}

int fgetc(FILE *stream)
{
    unsigned char byte;

    if (__mini_stdio_read(stream, &byte, 1) != 1) {
        return EOF;
    }
    return (int)byte;
}

int getc(FILE *stream)
{
    return fgetc(stream);
}

int getchar(void)
{
    return fgetc(stdin);
}

char *fgets(char *restrict s, int n, FILE *restrict stream)
{
    int count = 0;

    if (s == (char *)0 || n <= 0 || !readable_stream(stream) ||
        (stream->state & MINI_FILE_WRITE_NEEDS_SYNC) != 0U) {
        if (stream != (FILE *)0) {
            stream->state |= MINI_FILE_ERROR;
        }
        errno = EINVAL;
        return (char *)0;
    }

    if (n == 1) {
        s[0] = '\0';
        return s;
    }

    while (count < n - 1) {
        unsigned char byte;

        if (__mini_stdio_read(stream, &byte, 1) != 1) {
            break;
        }
        s[count++] = (char)byte;
        if (byte == (unsigned char)'\n') {
            break;
        }
    }

    if (ferror(stream)) {
        s[count] = '\0';
        return (char *)0;
    }
    if (count == 0 && feof(stream)) {
        return (char *)0;
    }

    s[count] = '\0';
    return s;
}

int ungetc(int c, FILE *stream)
{
    if (c == EOF) {
        return EOF;
    }
    if (!readable_stream(stream) ||
        (stream->state & MINI_FILE_WRITE_NEEDS_SYNC) != 0U ||
        stream->pushback_valid != 0U) {
        if (stream != (FILE *)0) {
            stream->state |= MINI_FILE_ERROR;
        }
        errno = EINVAL;
        return EOF;
    }

    stream->pushback_byte = (unsigned char)c;
    stream->pushback_valid = 1U;
    stream->state &= ~MINI_FILE_EOF;
    stream->state |= MINI_FILE_READ_NEEDS_POSITION;
    return (int)stream->pushback_byte;
}
