#include <errno.h>
#include <stdio.h>

#define MINI_STDIO_BYTE_PUBLIC_WRAPPER 1
#include "stdio_internal.h"

static int valid_stream(FILE *stream)
{
    return stream != (FILE *)0 &&
           (stream->mode & (MINI_FILE_READABLE | MINI_FILE_WRITABLE)) != 0U;
}

static int orientation_value(FILE *stream)
{
    unsigned int orientation = stream->state & MINI_FILE_ORIENTATION_MASK;

    if (orientation == MINI_FILE_WIDE_ORIENTED) {
        return 1;
    }
    if (orientation == MINI_FILE_BYTE_ORIENTED) {
        return -1;
    }
    return 0;
}

int __mini_stdio_fwide_unlocked(FILE *stream, int mode)
{
    int current;

    if (!valid_stream(stream)) {
        errno = EINVAL;
        return 0;
    }

    current = orientation_value(stream);
    if (current == 0 && mode != 0) {
        if (mode > 0) {
            stream->state |= MINI_FILE_WIDE_ORIENTED;
            current = 1;
        } else {
            stream->state |= MINI_FILE_BYTE_ORIENTED;
            current = -1;
        }
    }
    return current;
}

static int require_orientation(FILE *stream, unsigned int wanted)
{
    unsigned int current;

    if (!valid_stream(stream)) {
        if (stream != (FILE *)0) {
            stream->state |= MINI_FILE_ERROR;
        }
        errno = EINVAL;
        return EOF;
    }

    current = stream->state & MINI_FILE_ORIENTATION_MASK;
    if (current == 0U) {
        stream->state |= wanted;
        return 0;
    }
    if (current != wanted) {
        stream->state |= MINI_FILE_ERROR;
        errno = EINVAL;
        return EOF;
    }
    return 0;
}

int __mini_stdio_require_byte(FILE *stream)
{
    return require_orientation(stream, MINI_FILE_BYTE_ORIENTED);
}

int __mini_stdio_require_wide(FILE *stream)
{
    return require_orientation(stream, MINI_FILE_WIDE_ORIENTED);
}
