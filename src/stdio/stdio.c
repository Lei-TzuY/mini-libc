#include <errno.h>
#include <mini/syscall.h>
#include <stddef.h>
#include <stdio.h>

#include "stdio_internal.h"

static int mark_error(FILE *stream, int error)
{
    if (stream != (FILE *)0) {
        stream->state |= MINI_FILE_ERROR;
    }
    errno = error;
    return EOF;
}

int fgetc(FILE *stream)
{
    unsigned char byte;
    long result;

    if (stream == (FILE *)0 || (stream->mode & MINI_FILE_READABLE) == 0U) {
        return mark_error(stream, EINVAL);
    }
    if ((stream->state & MINI_FILE_WRITE_NEEDS_SYNC) != 0U) {
        return mark_error(stream, EINVAL);
    }
    if ((stream->state & MINI_FILE_EOF) != 0U) {
        return EOF;
    }

    result = mini_sys_read(stream->fd, &byte, 1);
    if (result < 0) {
        stream->state |= MINI_FILE_READ_NEEDS_POSITION;
        return mark_error(stream, (int)-result);
    }
    if (result == 0) {
        stream->state |= MINI_FILE_EOF;
        stream->state &= ~MINI_FILE_READ_NEEDS_POSITION;
        return EOF;
    }
    if (result != 1) {
        stream->state |= MINI_FILE_READ_NEEDS_POSITION;
        return mark_error(stream, EIO);
    }

    stream->state |= MINI_FILE_READ_NEEDS_POSITION;
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
