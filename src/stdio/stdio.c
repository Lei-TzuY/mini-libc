#include <errno.h>
#include <mini/syscall.h>
#include <stddef.h>
#include <stdio.h>

#include "stdio_internal.h"

static FILE mini_stderr = {
    2, MINI_FILE_WRITABLE | MINI_FILE_UNBUFFERED, 0, (FILE *)0, 0, {0}
};
static FILE mini_stdout = {
    1, MINI_FILE_WRITABLE, 0, &mini_stderr, 0, {0}
};
static FILE mini_stdin = {
    0, MINI_FILE_READABLE, 0, &mini_stdout, 0, {0}
};

FILE *__mini_stdin = &mini_stdin;
FILE *__mini_stdout = &mini_stdout;
FILE *__mini_stderr = &mini_stderr;

static FILE *mini_stream_head = &mini_stdin;

static int mark_error(FILE *stream, int error)
{
    if (stream != (FILE *)0) {
        stream->state |= MINI_FILE_ERROR;
    }
    errno = error;
    return EOF;
}

static size_t mark_write_error(FILE *stream, int error, size_t completed)
{
    (void)mark_error(stream, error);
    return completed;
}

static size_t write_raw(FILE *stream, const unsigned char *buffer, size_t length)
{
    size_t completed = 0;

    while (completed < length) {
        size_t remaining = length - completed;
        long result = mini_sys_write(stream->fd, buffer + completed,
                                     (unsigned long)remaining);

        if (result < 0) {
            return mark_write_error(stream, (int)-result, completed);
        }
        if (result == 0) {
            return mark_write_error(stream, EIO, completed);
        }
        if ((size_t)result > remaining) {
            return mark_write_error(stream, EIO, completed);
        }
        completed += (size_t)result;
    }

    return completed;
}

int __mini_stdio_flush_buffer(FILE *stream)
{
    size_t completed = 0;
    size_t length;

    if (stream == (FILE *)0 || (stream->mode & MINI_FILE_WRITABLE) == 0U) {
        return mark_error(stream, EINVAL);
    }

    length = stream->write_length;
    while (completed < length) {
        size_t remaining = length - completed;
        long result = mini_sys_write(stream->fd,
                                     stream->write_buffer + completed,
                                     (unsigned long)remaining);

        if (result < 0 || result == 0 || (size_t)result > remaining) {
            size_t i;
            int error = result < 0 ? (int)-result : EIO;

            for (i = 0; i < remaining; ++i) {
                stream->write_buffer[i] = stream->write_buffer[completed + i];
            }
            stream->write_length = remaining;
            return mark_error(stream, error);
        }
        completed += (size_t)result;
    }

    stream->write_length = 0;
    return 0;
}

size_t __mini_stdio_write(FILE *stream, const unsigned char *buffer,
                          size_t length)
{
    size_t accepted = 0;

    if (length == 0) {
        return 0;
    }
    if (stream == (FILE *)0 || (stream->mode & MINI_FILE_WRITABLE) == 0U) {
        return mark_write_error(stream, EINVAL, 0);
    }
    if ((stream->state & MINI_FILE_READ_NEEDS_POSITION) != 0U) {
        return mark_write_error(stream, EINVAL, 0);
    }

    stream->state |= MINI_FILE_WRITE_NEEDS_SYNC;
    if ((stream->mode & MINI_FILE_UNBUFFERED) != 0U) {
        return write_raw(stream, buffer, length);
    }

    while (accepted < length) {
        size_t space;
        size_t chunk;
        size_t i;

        if (stream->write_length == MINI_FILE_BUFFER_SIZE) {
            if (__mini_stdio_flush_buffer(stream) == EOF) {
                return accepted;
            }
        }

        space = MINI_FILE_BUFFER_SIZE - stream->write_length;
        chunk = length - accepted;
        if (chunk > space) {
            chunk = space;
        }
        for (i = 0; i < chunk; ++i) {
            stream->write_buffer[stream->write_length + i] = buffer[accepted + i];
        }
        stream->write_length += chunk;
        accepted += chunk;
    }

    return accepted;
}

void __mini_stdio_register(FILE *stream)
{
    stream->next = mini_stream_head;
    mini_stream_head = stream;
}

void __mini_stdio_unregister(FILE *stream)
{
    FILE **link = &mini_stream_head;

    while (*link != (FILE *)0) {
        if (*link == stream) {
            *link = stream->next;
            stream->next = (FILE *)0;
            return;
        }
        link = &(*link)->next;
    }
}

int fflush(FILE *stream)
{
    if (stream == (FILE *)0) {
        return __mini_stdio_flush_all();
    }
    if ((stream->mode & (MINI_FILE_READABLE | MINI_FILE_WRITABLE)) == 0U) {
        return mark_error(stream, EINVAL);
    }
    if ((stream->mode & MINI_FILE_WRITABLE) == 0U) {
        return 0;
    }
    if (__mini_stdio_flush_buffer(stream) == EOF) {
        return EOF;
    }

    stream->state &= ~MINI_FILE_WRITE_NEEDS_SYNC;
    return 0;
}

int __mini_stdio_flush_all(void)
{
    FILE *stream = mini_stream_head;
    int saved_errno = errno;
    int first_error = 0;

    while (stream != (FILE *)0) {
        FILE *next = stream->next;

        if ((stream->mode & MINI_FILE_WRITABLE) != 0U && fflush(stream) == EOF &&
            first_error == 0) {
            first_error = errno;
        }
        stream = next;
    }

    if (first_error != 0) {
        errno = first_error;
        return EOF;
    }
    errno = saved_errno;
    return 0;
}

int fputc(int c, FILE *stream)
{
    unsigned char byte = (unsigned char)c;

    if (__mini_stdio_write(stream, &byte, 1) != 1) {
        return EOF;
    }
    return (int)byte;
}

int putc(int c, FILE *stream)
{
    return fputc(c, stream);
}

int putchar(int c)
{
    return fputc(c, stdout);
}

int fputs(const char *restrict s, FILE *restrict stream)
{
    size_t length = 0;

    if (stream == (FILE *)0 || (stream->mode & MINI_FILE_WRITABLE) == 0U) {
        return mark_error(stream, EINVAL);
    }
    while (s[length] != '\0') {
        ++length;
    }
    if (__mini_stdio_write(stream, (const unsigned char *)s, length) != length) {
        return EOF;
    }
    return 0;
}

int puts(const char *s)
{
    if (fputs(s, stdout) == EOF || fputc('\n', stdout) == EOF) {
        return EOF;
    }
    return 0;
}

int feof(FILE *stream)
{
    return stream != (FILE *)0 && (stream->state & MINI_FILE_EOF) != 0U;
}

int ferror(FILE *stream)
{
    return stream != (FILE *)0 && (stream->state & MINI_FILE_ERROR) != 0U;
}

void clearerr(FILE *stream)
{
    if (stream != (FILE *)0) {
        stream->state &= ~(MINI_FILE_EOF | MINI_FILE_ERROR);
    }
}
