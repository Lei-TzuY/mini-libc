#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#define MINI_STDIO_BYTE_PUBLIC_WRAPPER 1
#include "../stdio/stdio_internal.h"

int __mini_vwscan_dispatch(FILE *stream, const char *format, va_list ap);
int __mini_vwsscan_dispatch(const wchar_t *input, const char *format, va_list ap);

static char *copy_wide_scan_format(const wchar_t *format)
{
    char *narrow;
    size_t length = 0U;
    size_t i;

    if (format == (const wchar_t *)0) {
        errno = EINVAL;
        return (char *)0;
    }
    while (format[length] != 0) {
        if (format[length] < 0 || (unsigned int)format[length] > 0x7fU) {
            errno = EILSEQ;
            return (char *)0;
        }
        ++length;
    }

    narrow = (char *)malloc(length + 1U);
    if (narrow == (char *)0) {
        return (char *)0;
    }
    for (i = 0U; i < length; ++i) {
        narrow[i] = (char)format[i];
    }
    narrow[length] = '\0';
    return narrow;
}

int vfwscanf(FILE *restrict stream, const wchar_t *restrict format, va_list ap)
{
    char *narrow = copy_wide_scan_format(format);
    int result;

    if (narrow == (char *)0) {
        return EOF;
    }

    __mini_stdio_lock();
    if (__mini_stdio_require_wide(stream) == EOF) {
        result = EOF;
    } else {
        result = __mini_vwscan_dispatch(stream, narrow, ap);
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
    char *narrow = copy_wide_scan_format(format);
    int result;

    if (input == (const wchar_t *)0) {
        free(narrow);
        errno = EINVAL;
        return EOF;
    }
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
