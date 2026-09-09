#ifndef MINI_LIBC_WCHAR_H
#define MINI_LIBC_WCHAR_H

#include <stddef.h>

typedef struct {
    unsigned int __count;
    unsigned int __value;
} mbstate_t;

int mbsinit(const mbstate_t *ps);
size_t mbrtowc(wchar_t *restrict pwc, const char *restrict s, size_t n,
               mbstate_t *restrict ps);
size_t wcrtomb(char *restrict s, wchar_t wc, mbstate_t *restrict ps);
size_t mbsrtowcs(wchar_t *restrict dst, const char **restrict src, size_t len,
                 mbstate_t *restrict ps);
size_t wcsrtombs(char *restrict dst, const wchar_t **restrict src, size_t len,
                 mbstate_t *restrict ps);

size_t wcslen(const wchar_t *s);
int wcscmp(const wchar_t *left, const wchar_t *right);
wchar_t *wcscpy(wchar_t *restrict dst, const wchar_t *restrict src);

#endif
