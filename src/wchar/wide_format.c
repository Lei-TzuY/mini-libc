#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#define MINI_STDIO_BYTE_PUBLIC_WRAPPER 1
#define MINI_STDIO_SYNC_PUBLIC_WRAPPER 1
#include "../locale/locale_internal.h"
#include "../stdio/format_internal.h"
#include "../stdio/stdio_internal.h"
#include "wchar_internal.h"

static int wide_stream_error(FILE *stream, int error)
{
    if (stream != (FILE *)0) {
        stream->state |= MINI_FILE_ERROR;
    }
    errno = error;
    return EOF;
}

static size_t encode_wide_value(wchar_t value, int utf8, char bytes[4])
{
    mbstate_t state = {0U, 0U};

    return __mini_wcrtomb_mode(bytes, value, &state, utf8);
}

static int encode_wide_format(const wchar_t *format, int utf8,
                              char **narrow_out)
{
    size_t length = 0U;
    size_t index;
    char *narrow;

    if (format == (const wchar_t *)0 || narrow_out == (char **)0) {
        errno = EINVAL;
        return EOF;
    }

    for (index = 0U; format[index] != 0; ++index) {
        char bytes[4];
        size_t converted = encode_wide_value(format[index], utf8, bytes);

        if (converted == (size_t)-1) {
            return EOF;
        }
        if (length > (size_t)-1 - converted - 1U) {
            errno = ENOMEM;
            return EOF;
        }
        length += converted;
    }

    narrow = (char *)malloc(length + 1U);
    if (narrow == (char *)0) {
        return EOF;
    }

    length = 0U;
    for (index = 0U; format[index] != 0; ++index) {
        char bytes[4];
        size_t converted = encode_wide_value(format[index], utf8, bytes);
        size_t i;

        if (converted == (size_t)-1) {
            free(narrow);
            return EOF;
        }
        for (i = 0U; i < converted; ++i) {
            narrow[length++] = bytes[i];
        }
    }
    narrow[length] = '\0';
    *narrow_out = narrow;
    return 0;
}

static int render_wide(const wchar_t *format, va_list ap, int utf8,
                       char **rendered_out)
{
    char *narrow_format = (char *)0;
    char *rendered = (char *)0;
    va_list copy;
    int count;
    int actual;
    int error;

    if (rendered_out == (char **)0 ||
        encode_wide_format(format, utf8, &narrow_format) == EOF) {
        return EOF;
    }

    va_copy(copy, ap);
    count = __mini_vsnprintf_wide_mode((char *)0, 0U, narrow_format,
                                       copy, utf8);
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
    actual = __mini_vsnprintf_wide_mode(rendered, (size_t)count + 1U,
                                        narrow_format, copy, utf8);
    va_end(copy);
    free(narrow_format);
    if (actual != count) {
        error = errno;
        free(rendered);
        errno = actual < 0 ? error : EINVAL;
        return EOF;
    }

    *rendered_out = rendered;
    return count;
}

static int decode_rendered_character(const char *rendered, size_t length,
                                     size_t *offset, int utf8,
                                     wchar_t *wide_out)
{
    mbstate_t state = {0U, 0U};

    while (*offset < length) {
        size_t converted = __mini_mbrtowc_mode(
            wide_out, rendered + *offset, 1U, &state, utf8);

        ++*offset;
        if (converted == (size_t)-1) {
            return 0;
        }
        if (converted != (size_t)-2) {
            return 1;
        }
    }

    errno = EILSEQ;
    return 0;
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
                                   size_t length, int utf8)
{
    size_t offset = 0U;
    unsigned int count = 0U;

    while (offset < length) {
        size_t start = offset;
        wchar_t wc;

        if (!decode_rendered_character(rendered, length, &offset, utf8, &wc)) {
            return wide_stream_error(stream, errno);
        }
        if (__mini_stdio_write(stream,
                (const unsigned char *)(rendered + start),
                offset - start) != offset - start) {
            return EOF;
        }
        ++count;
    }
    return (int)count;
}

int vfwprintf(FILE *restrict stream, const wchar_t *restrict format, va_list ap)
{
    char *rendered = (char *)0;
    int saved_errno = errno;
    int byte_count;
    int result;

    __mini_stdio_lock();
    if (wide_require_writable(stream) == EOF) {
        result = EOF;
    } else {
        int utf8 = (stream->state & MINI_FILE_WIDE_UTF8) != 0U;

        byte_count = render_wide(format, ap, utf8, &rendered);
        if (byte_count < 0) {
            if (errno == EILSEQ && stream != (FILE *)0) {
                stream->state |= MINI_FILE_ERROR;
            }
            result = EOF;
        } else {
            result = write_rendered_unlocked(stream, rendered,
                                             (size_t)byte_count, utf8);
            if (result >= 0) {
                errno = saved_errno;
            }
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

static int store_rendered_wide(wchar_t *buffer, size_t size,
                               const char *rendered, size_t length, int utf8)
{
    size_t offset = 0U;
    size_t count = 0U;
    size_t stored = 0U;

    while (offset < length) {
        wchar_t wc;

        if (!decode_rendered_character(rendered, length, &offset, utf8, &wc)) {
            if (size != 0U && buffer != (wchar_t *)0) {
                buffer[stored] = 0;
            }
            return EOF;
        }
        if (size != 0U && stored + 1U < size) {
            buffer[stored++] = wc;
        }
        ++count;
    }

    if (size != 0U && buffer != (wchar_t *)0) {
        buffer[stored] = 0;
    }
    if (size == 0U || count >= size) {
        errno = ERANGE;
        return EOF;
    }
    return (int)count;
}

int vswprintf(wchar_t *restrict buffer, size_t size,
              const wchar_t *restrict format, va_list ap)
{
    char *rendered = (char *)0;
    int saved_errno = errno;
    int byte_count;
    int result;
    int utf8;

    if (size != 0U && buffer == (wchar_t *)0) {
        errno = EINVAL;
        return EOF;
    }

    utf8 = __mini_locale_is_utf8();
    byte_count = render_wide(format, ap, utf8, &rendered);
    if (byte_count < 0) {
        return EOF;
    }

    result = store_rendered_wide(buffer, size, rendered,
                                 (size_t)byte_count, utf8);
    free(rendered);
    if (result >= 0) {
        errno = saved_errno;
    }
    return result;
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
