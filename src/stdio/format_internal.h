#ifndef MINI_LIBC_FORMAT_INTERNAL_H
#define MINI_LIBC_FORMAT_INTERNAL_H

#include <stdarg.h>
#include <stddef.h>

int __mini_vsnprintf_wide_mode(char *buffer, size_t size,
                               const char *format, va_list ap, int utf8);

#endif
