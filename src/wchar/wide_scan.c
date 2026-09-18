#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#define MINI_STDIO_BYTE_PUBLIC_WRAPPER 1
#include "../locale/locale_internal.h"
#include "../stdio/stdio_internal.h"
#include "wchar_internal.h"

int __mini_vwscan_dispatch(FILE *stream, const char *format, va_list ap);
int __mini_vwsscan_dispatch(const wchar_t *input, const char *format, va_list ap);

static size_t encode_wide_value(wchar_t value, int utf8, char bytes[4])
{
    mbstate_t state = {0U, 0U};

    return __mini_wcrtomb_mode(bytes, value, &state, utf8);
}

static char *encode_wide_scan_format(const wchar_t *format, int utf8)
{
    char *narrow;
    size_t length = 0U;
    size_t index;

    if (format == (const wchar_t *)0) {
        errno = EINVAL;
        return (char *)0;
    }

    for (index = 0U; format[index] != 0; ++index) {
        char bytes[4];
        size_t converted = encode_wide_value(format[index], utf8, bytes);

        if (converted == (size_t)-1) {
            return (char *)0;
        }
        if (length > (size_t)-1 - converted - 1U) {
            errno = ENOMEM;
            return (char *)0;
        }
        length += converted;
    }

    narrow = (char *)malloc(length + 1U);
    if (narrow == (char *)0) {
        return (char *)0;
    }

    length = 0U;
    for (index = 0U; format[index] != 0; ++index) {
        char bytes[4];
        size_t converted = encode_wide_value(format[index], utf8, bytes);
        size_t i;

        if (converted == (size_t)-1) {
            free(narrow);
            return (char *)0;
        }
        for (i = 0U; i < converted; ++i) {
            narrow[length++] = bytes[i];
        }
    }
    narrow[length] = '\0';
    return narrow;
}

int vfwscanf(FILE *restrict stream, const wchar_t *restrict format, va_list ap)
{
    char *narrow = (char *)0;
    int result;

    __mini_stdio_lock();
    if (__mini_stdio_require_wide(stream) == EOF) {
        result = EOF;
    } else {
        int utf8 = (stream->state & MINI_FILE_WIDE_UTF8) != 0U;

        narrow = encode_wide_scan_format(format, utf8);
        if (narrow == (char *)0) {
            result = EOF;
        } else {
            result = __mini_vwscan_dispatch(stream, narrow, ap);
        }
    }
    __mini_stdio_unlock();
    free(narrow);
    return result;
}

int fwscanf(FILE *restrict stream, const wchar_t *restrict format, ...)
{
    va_list ap;
    int result;

    va_start(ap, format);
    result = vfwscanf(stream, format, ap);
    va_end(ap);
    return result;
}

int vwscanf(const wchar_t *restrict format, va_list ap)
{
    return vfwscanf(stdin, format, ap);
}

int wscanf(const wchar_t *restrict format, ...)
{
    va_list ap;
    int result;

    va_start(ap, format);
    result = vwscanf(format, ap);
    va_end(ap);
    return result;
}

int vswscanf(const wchar_t *restrict input, const wchar_t *restrict format,
             va_list ap)
{
    char *narrow;
    int result;

    if (input == (const wchar_t *)0) {
        errno = EINVAL;
        return EOF;
    }

    narrow = encode_wide_scan_format(format, __mini_locale_is_utf8());
    if (narrow == (char *)0) {
        return EOF;
    }

    result = __mini_vwsscan_dispatch(input, narrow, ap);
    free(narrow);
    return result;
}

int swscanf(const wchar_t *restrict input, const wchar_t *restrict format, ...)
{
    va_list ap;
    int result;

    va_start(ap, format);
    result = vswscanf(input, format, ap);
    va_end(ap);
    return result;
}
