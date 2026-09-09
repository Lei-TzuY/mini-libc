#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#define MINI_STDIO_BYTE_PUBLIC_WRAPPER 1
#define MINI_STDIO_SYNC_PUBLIC_WRAPPER 1
#include "../stdio/stdio_internal.h"

static int wide_stream_error(FILE *stream, int error)
{
    if (stream != (FILE *)0) {
        stream->state |= MINI_FILE_ERROR;
    }
    errno = error;
    return EOF;
}

static int copy_wide_format(const wchar_t *format, char **narrow_out)
{
    size_t length = 0U;
    size_t i;
    char *narrow;

    if (format == (const wchar_t *)0 || narrow_out == (char **)0) {
        errno = EINVAL;
        return EOF;
    }

    while (format[length] != 0) {
        wchar_t wc = format[length];

        if (wc < 0 || (unsigned int)wc > 0x7fU) {
            errno = EILSEQ;
            return EOF;
        }
        ++length;
    }
    if (length == (size_t)-1) {
        errno = ENOMEM;
        return EOF;
    }

    narrow = (char *)malloc(length + 1U);
    if (narrow == (char *)0) {
        return EOF;
    }
    for (i = 0U; i < length; ++i) {
        narrow[i] = (char)format[i];
    }
    narrow[length] = '\0';
    *narrow_out = narrow;
    return 0;
}

static int render_ascii(const wchar_t *format, va_list ap,
                        char **rendered_out)
{
    char *narrow_format = (char *)0;
    char *rendered = (char *)0;
    va_list copy;
    int count;
    int actual;
    int error;
    size_t i;

    if (rendered_out == (char **)0 ||
        copy_wide_format(format, &narrow_format) == EOF) {
        return EOF;
    }

    va_copy(copy, ap);
    count = vsnprintf((char *)0, 0U, narrow_format, copy);
    va_end(copy);
    if (count < 0) {
        free(narrow_format);
        return EOF;
    }

    rendered = (char *)malloc((size_t)count + 1U);
    if (rendered == (char *)0) {
        free(narrow_format);
        return EOF;
    }

    va_copy(copy, ap);
    actual = vsnprintf(rendered, (size_t)count + 1U, narrow_format, copy);
    va_end(copy);
    free(narrow_format);
    if (actual != count) {
        error = errno;
        free(rendered);
        errno = actual < 0 ? error : EINVAL;
        return EOF;
    }

    for (i = 0U; i < (size_t)count; ++i) {
        if ((unsigned char)rendered[i] > 0x7fU) {
            free(rendered);
            errno = EILSEQ;
            return EOF;
        }
    }

    *rendered_out = rendered;
    return count;
}

static int wide_require_writable(FILE *stream)
{
    if (__mini_stdio_require_wide(stream) == EOF) {
        return EOF;
    }
    if ((stream->mode & MINI_FILE_WRITABLE) == 0U) {
        return wide_stream_error(stream, EINVAL);
    }
    return 0;
}

static int write_rendered_unlocked(FILE *stream, const char *rendered,
                                   size_t length)
{
    mbstate_t state = {0U, 0U};
    size_t i;

    for (i = 0U; i < length; ++i) {
        char byte;
        size_t converted = wcrtomb(&byte,
                                   (wchar_t)(unsigned char)rendered[i],
                                   &state);

        if (converted == (size_t)-1) {
            return wide_stream_error(stream, errno);
        }
        if (converted != 1U) {
            return wide_stream_error(stream, EILSEQ);
        }
        if (__mini_stdio_write(stream, (const unsigned char *)&byte, 1U) != 1U) {
            return EOF;
        }
    }
    return 0;
}

int vfwprintf(FILE *restrict stream, const wchar_t *restrict format, va_list ap)
{
    char *rendered = (char *)0;
    int saved_errno = errno;
    int count;
    int result;

    __mini_stdio_lock();
    if (wide_require_writable(stream) == EOF) {
        result = EOF;
    } else {
        count = render_ascii(format, ap, &rendered);
        if (count < 0) {
            if (errno == EILSEQ && stream != (FILE *)0) {
                stream->state |= MINI_FILE_ERROR;
            }
            result = EOF;
        } else if (write_rendered_unlocked(stream, rendered,
                                           (size_t)count) == EOF) {
            result = EOF;
        } else {
            result = count;
            errno = saved_errno;
        }
    }
    __mini_stdio_unlock();
    free(rendered);
    return result;
}

int vwprintf(const wchar_t *restrict format, va_list ap)
{
    return vfwprintf(stdout, format, ap);
}

int fwprintf(FILE *restrict stream, const wchar_t *restrict format, ...)
{
    va_list ap;
    int result;

    va_start(ap, format);
    result = vfwprintf(stream, format, ap);
    va_end(ap);
    return result;
}

int wprintf(const wchar_t *restrict format, ...)
{
    va_list ap;
    int result;

    va_start(ap, format);
    result = vwprintf(format, ap);
    va_end(ap);
    return result;
}

int vswprintf(wchar_t *restrict buffer, size_t size,
              const wchar_t *restrict format, va_list ap)
{
    char *rendered = (char *)0;
    int saved_errno = errno;
    int count;
    size_t copied;
    size_t i;

    if (size != 0U && buffer == (wchar_t *)0) {
        errno = EINVAL;
        return EOF;
    }

    count = render_ascii(format, ap, &rendered);
    if (count < 0) {
        return EOF;
    }

    if (size == 0U || (size_t)count >= size) {
        copied = size == 0U ? 0U : size - 1U;
        if (buffer != (wchar_t *)0) {
            for (i = 0U; i < copied; ++i) {
                buffer[i] = (wchar_t)(unsigned char)rendered[i];
            }
            buffer[copied] = 0;
        }
        free(rendered);
        errno = ERANGE;
        return EOF;
    }

    for (i = 0U; i < (size_t)count; ++i) {
        buffer[i] = (wchar_t)(unsigned char)rendered[i];
    }
    buffer[count] = 0;
    free(rendered);
    errno = saved_errno;
    return count;
}

int swprintf(wchar_t *restrict buffer, size_t size,
             const wchar_t *restrict format, ...)
{
    va_list ap;
    int result;

    va_start(ap, format);
    result = vswprintf(buffer, size, format, ap);
    va_end(ap);
    return result;
}
