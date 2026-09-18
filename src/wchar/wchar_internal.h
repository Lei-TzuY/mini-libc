#ifndef MINI_LIBC_WCHAR_INTERNAL_H
#define MINI_LIBC_WCHAR_INTERNAL_H

#include <stddef.h>
#include <wchar.h>

size_t __mini_mbrtowc_mode(wchar_t *restrict pwc, const char *restrict s,
                           size_t n, mbstate_t *restrict ps, int utf8);
size_t __mini_wcrtomb_mode(char *restrict s, wchar_t wc,
                           mbstate_t *restrict ps, int utf8);

#endif
