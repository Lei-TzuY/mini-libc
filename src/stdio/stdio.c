#include <errno.h>
#include <mini/syscall.h>
#include <stddef.h>
#include <stdio.h>

#include "stdio_internal.h"

static FILE mini_stderr = {
    2, MINI_FILE_WRITABLE | MINI_FILE_UNBUFFERED, 0, (FILE *)0,
    0, (unsigned char *)0, 0, 0, 0U, 0U, (unsigned char *)0, 0,
    {0}, {0}
};
static FILE mini_stdout = {
    1, MINI_FILE_WRITABLE, 0, &mini_stderr,
    0, (unsigned char *)0, 0, 0, 0U, 0U, (unsigned char *)0, 0,
    {0}, {0}
};
static FILE mini_stdin = {
    0, MINI_FILE_READABLE, 0, &mini_stdout,
    0, (unsigned char *)0, 0, 0, 0U, 0U, (unsigned char *)0, 0,
    {0}, {0}
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

static size_t mark_read_error(FILE *stream, int error, size_t completed)
{
    (void)mark_error(stream, error);
    return completed;
}

static size_t mark_write_error(FILE *stream, int error, size_t completed)
{
    (void)mark_error(stream, error);
    return completed;
}

static int readable_stream(FILE *stream)
{
    return stream != (FILE *)0 && (stream->mode & MINI_FILE_READABLE) != 0U;
}

static void ensure_storage(FILE *stream)
{
    if (stream->buffer_size == 0U) {
        stream->buffer_size = MINI_FILE_BUFFER_SIZE;
    }
    if (stream->write_buffer == (unsigned char *)0) {
        stream->write_buffer = stream->inline_write_buffer;
    }
    if (stream->read_buffer == (unsigned char *)0) {
        stream->read_buffer = stream->inline_read_buffer;
    }
}

static size_t stdio_read_unlocked(FILE *stream, unsigned char *buffer,
                                  size_t length)
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

    if ((stream->mode & MINI_FILE_UNBUFFERED) != 0U) {
        while (completed < length) {
            size_t remaining = length - completed;
            long result = mini_sys_read(stream->fd, buffer + completed,
                                        (unsigned long)remaining);

            if (result < 0) {
                stream->state |= MINI_FILE_READ_NEEDS_POSITION;
                return mark_read_error(stream, (int)-result, completed);
            }
            if (result == 0) {
                stream->state |= MINI_FILE_EOF;
                stream->state &= ~MINI_FILE_READ_NEEDS_POSITION;
                return completed;
            }
            if ((size_t)result > remaining) {
                stream->state |= MINI_FILE_READ_NEEDS_POSITION;
                return mark_read_error(stream, EIO, completed);
            }
            completed += (size_t)result;
            stream->state |= MINI_FILE_READ_NEEDS_POSITION;
        }
        return completed;
    }

    ensure_storage(stream);
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
                                        (unsigned long)stream->buffer_size);

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
            if ((size_t)result > stream->buffer_size) {
                stream->state |= MINI_FILE_READ_NEEDS_POSITION;
                return mark_read_error(stream, EIO, completed);
            }

            stream->read_offset = 0;
            stream->read_length = (size_t)result;
        }
    }

    return completed;
}

size_t __mini_stdio_read(FILE *stream, unsigned char *buffer, size_t length)
{
    size_t result;

    __mini_stdio_lock();
    result = stdio_read_unlocked(stream, buffer, length);
    __mini_stdio_unlock();
    return result;
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

static int stdio_flush_buffer_unlocked(FILE *stream)
{
    size_t completed = 0;
    size_t length;

    if (stream == (FILE *)0 || (stream->mode & MINI_FILE_WRITABLE) == 0U) {
        return mark_error(stream, EINVAL);
    }

    length = stream->write_length;
    if (length == 0U) {
        return 0;
    }
    ensure_storage(stream);
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

int __mini_stdio_flush_buffer(FILE *stream)
{
    int result;

    __mini_stdio_lock();
    result = stdio_flush_buffer_unlocked(stream);
    __mini_stdio_unlock();
    return result;
}

static size_t stdio_write_unlocked(FILE *stream, const unsigned char *buffer,
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

    ensure_storage(stream);
    while (accepted < length) {
        size_t space;
        size_t chunk;
        size_t i;
        int flush_line = 0;

        if (stream->write_length == stream->buffer_size) {
            if (stdio_flush_buffer_unlocked(stream) == EOF) {
                return accepted;
            }
        }

        space = stream->buffer_size - stream->write_length;
        chunk = length - accepted;
        if (chunk > space) {
            chunk = space;
        }
        if ((stream->mode & MINI_FILE_LINE_BUFFERED) != 0U) {
            for (i = 0; i < chunk; ++i) {
                if (buffer[accepted + i] == (unsigned char)'\n') {
                    chunk = i + 1U;
                    flush_line = 1;
                    break;
                }
            }
        }
        for (i = 0; i < chunk; ++i) {
            stream->write_buffer[stream->write_length + i] = buffer[accepted + i];
        }
        stream->write_length += chunk;
        accepted += chunk;

        if (flush_line && stdio_flush_buffer_unlocked(stream) == EOF) {
            return accepted;
        }
    }

    return accepted;
}

size_t __mini_stdio_write(FILE *stream, const unsigned char *buffer,
                          size_t length)
{
    size_t result;

    __mini_stdio_lock();
    result = stdio_write_unlocked(stream, buffer, length);
    __mini_stdio_unlock();
    return result;
}

void __mini_stdio_register(FILE *stream)
{
    __mini_stdio_lock();
    stream->next = mini_stream_head;
    mini_stream_head = stream;
    __mini_stdio_unlock();
}

void __mini_stdio_unregister(FILE *stream)
{
    FILE **link;

    __mini_stdio_lock();
    link = &mini_stream_head;
    while (*link != (FILE *)0) {
        if (*link == stream) {
            *link = stream->next;
            stream->next = (FILE *)0;
            break;
        }
        link = &(*link)->next;
    }
    __mini_stdio_unlock();
}

int fflush(FILE *stream)
{
    int result = 0;

    __mini_stdio_lock();
    if (stream == (FILE *)0) {
        result = __mini_stdio_flush_all();
    } else if ((stream->mode & (MINI_FILE_READABLE | MINI_FILE_WRITABLE)) == 0U) {
        result = mark_error(stream, EINVAL);
    } else if ((stream->mode & MINI_FILE_WRITABLE) != 0U) {
        result = stdio_flush_buffer_unlocked(stream);
        if (result != EOF) {
            stream->state &= ~MINI_FILE_WRITE_NEEDS_SYNC;
        }
    }
    __mini_stdio_unlock();
    return result;
}

int __mini_stdio_flush_all(void)
{
    FILE *stream;
    int saved_errno;
    int first_error = 0;

    __mini_stdio_lock();
    stream = mini_stream_head;
    saved_errno = errno;
    while (stream != (FILE *)0) {
        FILE *next = stream->next;

        if ((stream->mode & MINI_FILE_WRITABLE) != 0U &&
            stdio_flush_buffer_unlocked(stream) == EOF && first_error == 0) {
            first_error = errno;
        }
        stream = next;
    }

    if (first_error != 0) {
        errno = first_error;
        __mini_stdio_unlock();
        return EOF;
    }
    errno = saved_errno;
    __mini_stdio_unlock();
    return 0;
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

static char *fgets_unlocked(char *restrict s, int n, FILE *restrict stream)
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
        int value = fgetc(stream);

        if (value == EOF) {
            if (!feof(stream)) {
                s[count] = '\0';
                return (char *)0;
            }
            break;
        }
        s[count++] = (char)(unsigned char)value;
        if (value == '\n') {
            break;
        }
    }

    if (count == 0 && feof(stream)) {
        return (char *)0;
    }

    s[count] = '\0';
    return s;
}

char *fgets(char *restrict s, int n, FILE *restrict stream)
{
    char *result;

    __mini_stdio_lock();
    result = fgets_unlocked(s, n, stream);
    __mini_stdio_unlock();
    return result;
}

int ungetc(int c, FILE *stream)
{
    int result;

    __mini_stdio_lock();
    if (c == EOF) {
        result = EOF;
    } else if (!readable_stream(stream) ||
               (stream->state & MINI_FILE_WRITE_NEEDS_SYNC) != 0U ||
               stream->pushback_valid != 0U) {
        if (stream != (FILE *)0) {
            stream->state |= MINI_FILE_ERROR;
        }
        errno = EINVAL;
        result = EOF;
    } else {
        stream->pushback_byte = (unsigned char)c;
        stream->pushback_valid = 1U;
        stream->state &= ~MINI_FILE_EOF;
        stream->state |= MINI_FILE_READ_NEEDS_POSITION;
        result = (int)stream->pushback_byte;
    }
    __mini_stdio_unlock();
    return result;
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
    int result = 0;

    __mini_stdio_lock();
    if (stream == (FILE *)0 || (stream->mode & MINI_FILE_WRITABLE) == 0U) {
        result = mark_error(stream, EINVAL);
    } else {
        while (s[length] != '\0') {
            ++length;
        }
        if (stdio_write_unlocked(stream, (const unsigned char *)s, length) != length) {
            result = EOF;
        }
    }
    __mini_stdio_unlock();
    return result;
}

int puts(const char *s)
{
    int result = 0;

    __mini_stdio_lock();
    if (fputs(s, stdout) == EOF || fputc('\n', stdout) == EOF) {
        result = EOF;
    }
    __mini_stdio_unlock();
    return result;
}

int feof(FILE *stream)
{
    int result;

    __mini_stdio_lock();
    result = stream != (FILE *)0 && (stream->state & MINI_FILE_EOF) != 0U;
    __mini_stdio_unlock();
    return result;
}

int ferror(FILE *stream)
{
    int result;

    __mini_stdio_lock();
    result = stream != (FILE *)0 && (stream->state & MINI_FILE_ERROR) != 0U;
    __mini_stdio_unlock();
    return result;
}

void clearerr(FILE *stream)
{
    __mini_stdio_lock();
    if (stream != (FILE *)0) {
        stream->state &= ~(MINI_FILE_EOF | MINI_FILE_ERROR);
    }
    __mini_stdio_unlock();
}
