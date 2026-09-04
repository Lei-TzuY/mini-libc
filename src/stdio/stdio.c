#include <errno.h>
#include <mini/syscall.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define MINI_FILE_READABLE 1U
#define MINI_FILE_WRITABLE 2U
#define MINI_FILE_OWNED 4U
#define MINI_FILE_EOF 4U
#define MINI_FILE_ERROR 8U

#define MINI_AT_FDCWD (-100)
#define MINI_O_RDONLY 0
#define MINI_O_WRONLY 1
#define MINI_O_RDWR 2
#define MINI_O_CREAT 64
#define MINI_O_TRUNC 512
#define MINI_O_APPEND 1024

struct __mini_FILE {
    int fd;
    unsigned int mode;
    unsigned int state;
};

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

static int parse_open_mode(const char *mode, int *flags,
                           unsigned int *stream_mode)
{
    int plus = 0;
    int binary = 0;
    char kind;
    const char *cursor;

    if (mode == (const char *)0 || mode[0] == '\0') {
        return 0;
    }

    kind = mode[0];
    if (kind != 'r' && kind != 'w' && kind != 'a') {
        return 0;
    }

    cursor = mode + 1;
    while (*cursor != '\0') {
        if (*cursor == '+') {
            if (plus) {
                return 0;
            }
            plus = 1;
        } else if (*cursor == 'b') {
            if (binary) {
                return 0;
            }
            binary = 1;
        } else {
            return 0;
        }
        ++cursor;
    }

    if (plus) {
        *flags = MINI_O_RDWR;
        *stream_mode = MINI_FILE_READABLE | MINI_FILE_WRITABLE | MINI_FILE_OWNED;
    } else if (kind == 'r') {
        *flags = MINI_O_RDONLY;
        *stream_mode = MINI_FILE_READABLE | MINI_FILE_OWNED;
    } else {
        *flags = MINI_O_WRONLY;
        *stream_mode = MINI_FILE_WRITABLE | MINI_FILE_OWNED;
    }

    if (kind == 'w') {
        *flags |= MINI_O_CREAT | MINI_O_TRUNC;
    } else if (kind == 'a') {
        *flags |= MINI_O_CREAT | MINI_O_APPEND;
    }

    return 1;
}

FILE *fopen(const char *restrict filename, const char *restrict mode)
{
    FILE *stream;
    unsigned int stream_mode;
    int flags;
    long result;

    if (filename == (const char *)0 ||
        !parse_open_mode(mode, &flags, &stream_mode)) {
        errno = EINVAL;
        return (FILE *)0;
    }

    stream = (FILE *)malloc(sizeof(*stream));
    if (stream == (FILE *)0) {
        return (FILE *)0;
    }

    result = mini_sys_openat(MINI_AT_FDCWD, filename, flags, 0666U);
    if (result < 0) {
        int error = (int)-result;

        free(stream);
        errno = error;
        return (FILE *)0;
    }

    stream->fd = (int)result;
    stream->mode = stream_mode;
    stream->state = 0;
    return stream;
}

int fclose(FILE *stream)
{
    unsigned int owned;
    long result;

    if (stream == (FILE *)0 ||
        (stream->mode & (MINI_FILE_READABLE | MINI_FILE_WRITABLE)) == 0U) {
        return mark_error(stream, EINVAL);
    }

    owned = stream->mode & MINI_FILE_OWNED;
    result = mini_sys_close(stream->fd);

    if (owned) {
        int error = result < 0 ? (int)-result : 0;

        free(stream);
        if (result < 0) {
            errno = error;
            return EOF;
        }
        return 0;
    }

    stream->fd = -1;
    stream->mode = 0;
    stream->state = 0;
    if (result < 0) {
        errno = (int)-result;
        return EOF;
    }
    return 0;
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
