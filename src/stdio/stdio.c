#include <errno.h>
#include <mini/syscall.h>
#include <stddef.h>
#include <stdio.h>

#include "stdio_internal.h"

static FILE mini_stdin = {0, MINI_FILE_READABLE, 0};
static FILE mini_stdout = {1, MINI_FILE_WRITABLE, 0};
static FILE mini_stderr = {2, MINI_FILE_WRITABLE, 0};

FILE *__mini_stdin = &mini_stdin;
FILE *__mini_stdout = &mini_stdout;
FILE *__mini_stderr = &mini_stderr;

static int mark_error(FILE *stream, int error)
{
    if (stream != (FILE *)0) {
        stream->state |= MINI_FILE_ERROR;
    }
    errno = error;
    return EOF;
}

static int write_all(FILE *stream, const char *buffer, size_t length)
{
    if (stream == (FILE *)0 || (stream->mode & MINI_FILE_WRITABLE) == 0U) {
        return mark_error(stream, EINVAL);
    }

    while (length != 0) {
        long result = mini_sys_write(stream->fd, buffer, (unsigned long)length);

        if (result < 0) {
            return mark_error(stream, (int)-result);
        }
        if (result == 0) {
            return mark_error(stream, EIO);
        }
        buffer += (size_t)result;
        length -= (size_t)result;
    }
    return 0;
}

int fgetc(FILE *stream)
{
    unsigned char byte;
    long result;

    if (stream == (FILE *)0 || (stream->mode & MINI_FILE_READABLE) == 0U) {
        return mark_error(stream, EINVAL);
    }
    if ((stream->state & MINI_FILE_EOF) != 0U) {
        return EOF;
    }

    result = mini_sys_read(stream->fd, &byte, 1);
    if (result < 0) {
        return mark_error(stream, (int)-result);
    }
    if (result == 0) {
        stream->state |= MINI_FILE_EOF;
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

int fputc(int c, FILE *stream)
{
    unsigned char byte = (unsigned char)c;

    if (write_all(stream, (const char *)&byte, 1) == EOF) {
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

    while (s[length] != '\0') {
        ++length;
    }
    if (write_all(stream, s, length) == EOF) {
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
