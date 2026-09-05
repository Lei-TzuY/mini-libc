#include <errno.h>
#include <mini/syscall.h>
#include <stdio.h>
#include <stdlib.h>

#include "stdio_internal.h"

#define MINI_AT_FDCWD (-100)
#define MINI_AT_REMOVEDIR 512
#define MINI_EISDIR 21
#define MINI_O_RDONLY 0
#define MINI_O_WRONLY 1
#define MINI_O_RDWR 2
#define MINI_O_CREAT 64
#define MINI_O_EXCL 128
#define MINI_O_TRUNC 512
#define MINI_O_APPEND 1024
#define MINI_O_TMPFILE 4259840

static int parse_open_mode(const char *mode, int *flags,
                           unsigned int *stream_mode)
{
    int plus = 0;
    int binary = 0;
    int exclusive = 0;
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
        } else if (*cursor == 'x') {
            if (exclusive) {
                return 0;
            }
            exclusive = 1;
        } else {
            return 0;
        }
        ++cursor;
    }

    if (exclusive && kind != 'w') {
        return 0;
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
        if (exclusive) {
            *flags |= MINI_O_EXCL;
        }
    } else if (kind == 'a') {
        *flags |= MINI_O_CREAT | MINI_O_APPEND;
        *stream_mode |= MINI_FILE_APPEND;
    }

    return 1;
}

static void initialize_owned_stream(FILE *stream, int fd,
                                    unsigned int stream_mode)
{
    stream->fd = fd;
    stream->mode = stream_mode;
    stream->state = 0;
    stream->next = (FILE *)0;
    stream->write_length = 0;
    stream->write_buffer = (unsigned char *)0;
    stream->read_offset = 0;
    stream->read_length = 0;
    stream->pushback_valid = 0U;
    stream->pushback_byte = 0U;
    stream->read_buffer = (unsigned char *)0;
    stream->buffer_size = 0;
    __mini_stdio_register(stream);
}

void __mini_stdio_release_buffer(FILE *stream)
{
    if (stream == (FILE *)0) {
        return;
    }
    if ((stream->mode & MINI_FILE_BUFFER_OWNED) != 0U &&
        stream->write_buffer != (unsigned char *)0) {
        free(stream->write_buffer);
    }
    stream->mode &= ~MINI_FILE_BUFFER_OWNED;
    stream->write_buffer = (unsigned char *)0;
    stream->read_buffer = (unsigned char *)0;
    stream->buffer_size = 0;
}

int setvbuf(FILE *restrict stream, char *restrict buf, int mode, size_t size)
{
    unsigned char *new_buffer = (unsigned char *)buf;
    unsigned int new_owned = 0U;
    int saved_errno = errno;
    int sync_error;

    if (stream == (FILE *)0 ||
        (stream->mode & (MINI_FILE_READABLE | MINI_FILE_WRITABLE)) == 0U ||
        (mode != _IOFBF && mode != _IOLBF && mode != _IONBF) ||
        (mode != _IONBF && size == 0U)) {
        errno = EINVAL;
        return EOF;
    }

    if (mode != _IONBF && new_buffer == (unsigned char *)0) {
        new_buffer = (unsigned char *)malloc(size);
        if (new_buffer == (unsigned char *)0) {
            return EOF;
        }
        new_owned = MINI_FILE_BUFFER_OWNED;
    }

    if ((stream->mode & MINI_FILE_WRITABLE) != 0U && fflush(stream) == EOF) {
        sync_error = errno;
        if (new_owned != 0U) {
            free(new_buffer);
        }
        errno = sync_error;
        return EOF;
    }
    if ((stream->mode & MINI_FILE_READABLE) != 0U &&
        ((stream->state & MINI_FILE_READ_NEEDS_POSITION) != 0U ||
         stream->read_offset != stream->read_length ||
         stream->pushback_valid != 0U) &&
        fseek(stream, 0L, SEEK_CUR) != 0) {
        sync_error = errno;
        if (new_owned != 0U) {
            free(new_buffer);
        }
        errno = sync_error;
        return EOF;
    }

    __mini_stdio_release_buffer(stream);
    stream->mode &= ~(MINI_FILE_UNBUFFERED | MINI_FILE_LINE_BUFFERED);
    if (mode == _IONBF) {
        stream->mode |= MINI_FILE_UNBUFFERED;
    } else if (mode == _IOLBF) {
        stream->mode |= MINI_FILE_LINE_BUFFERED;
    }
    stream->mode |= new_owned;
    if (mode != _IONBF) {
        stream->write_buffer = new_buffer;
        stream->read_buffer = new_buffer;
        stream->buffer_size = size;
    }
    stream->write_length = 0;
    stream->read_offset = 0;
    stream->read_length = 0;
    stream->pushback_valid = 0U;
    stream->pushback_byte = 0U;
    errno = saved_errno;
    return 0;
}

void setbuf(FILE *restrict stream, char *restrict buf)
{
    if (buf == (char *)0) {
        (void)setvbuf(stream, (char *)0, _IONBF, 0U);
    } else {
        (void)setvbuf(stream, buf, _IOFBF, BUFSIZ);
    }
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

    initialize_owned_stream(stream, (int)result, stream_mode);
    return stream;
}

FILE *tmpfile(void)
{
    FILE *stream;
    long result;

    stream = (FILE *)malloc(sizeof(*stream));
    if (stream == (FILE *)0) {
        return (FILE *)0;
    }

    result = mini_sys_openat(MINI_AT_FDCWD, "/tmp",
                             MINI_O_RDWR | MINI_O_TMPFILE, 0600U);
    if (result < 0) {
        int error = (int)-result;

        free(stream);
        errno = error;
        return (FILE *)0;
    }

    initialize_owned_stream(stream, (int)result,
                            MINI_FILE_READABLE | MINI_FILE_WRITABLE |
                                MINI_FILE_OWNED);
    return stream;
}

/*
 * The hosted FILE-object harness compiles this translation unit with
 * mini_sys_openat macro-renamed to a deterministic fake and intentionally
 * isolates stream ownership/buffering from pathname mutation. Real-kernel and
 * cross-toolchain probes exercise the public pathname operations below.
 */
#ifndef mini_sys_openat
int remove(const char *filename)
{
    long result;

    if (filename == (const char *)0) {
        errno = EINVAL;
        return -1;
    }

    result = mini_sys_unlinkat(MINI_AT_FDCWD, filename, 0);
    if (result == -MINI_EISDIR) {
        result = mini_sys_unlinkat(MINI_AT_FDCWD, filename, MINI_AT_REMOVEDIR);
    }
    if (result < 0) {
        errno = (int)-result;
        return -1;
    }
    return 0;
}

int rename(const char *oldname, const char *newname)
{
    long result;

    if (oldname == (const char *)0 || newname == (const char *)0) {
        errno = EINVAL;
        return -1;
    }

    result = mini_sys_renameat(MINI_AT_FDCWD, oldname, MINI_AT_FDCWD, newname);
    if (result < 0) {
        errno = (int)-result;
        return -1;
    }
    return 0;
}
#endif

int fclose(FILE *stream)
{
    unsigned int owned;
    int first_error = 0;
    long result;

    if (stream == (FILE *)0 ||
        (stream->mode & (MINI_FILE_READABLE | MINI_FILE_WRITABLE)) == 0U) {
        if (stream != (FILE *)0) {
            stream->state |= MINI_FILE_ERROR;
        }
        errno = EINVAL;
        return EOF;
    }

    owned = stream->mode & MINI_FILE_OWNED;
    if ((stream->mode & MINI_FILE_WRITABLE) != 0U &&
        __mini_stdio_flush_buffer(stream) == EOF) {
        first_error = errno;
    }

    result = mini_sys_close(stream->fd);
    if (result < 0 && first_error == 0) {
        first_error = (int)-result;
    }

    __mini_stdio_unregister(stream);
    __mini_stdio_release_buffer(stream);
    if (owned) {
        free(stream);
    } else {
        stream->fd = -1;
        stream->mode = 0;
        stream->state = 0;
        stream->next = (FILE *)0;
        stream->write_length = 0;
        stream->read_offset = 0;
        stream->read_length = 0;
        stream->pushback_valid = 0U;
        stream->pushback_byte = 0U;
    }

    if (first_error != 0) {
        errno = first_error;
        return EOF;
    }
    return 0;
}
