#ifndef MINI_LIBC_WCHAR_H
#define MINI_LIBC_WCHAR_H

#include <stddef.h>
#include <stdio.h>

typedef unsigned int wint_t;

typedef struct {
    unsigned int __count;
    unsigned int __value;
} mbstate_t;

#define WEOF ((wint_t)-1)

int mbsinit(const mbstate_t *ps);
size_t mbrtowc(wchar_t *restrict pwc, const char *restrict s, size_t n,
               mbstate_t *restrict ps);
size_t wcrtomb(char *restrict s, wchar_t wc, mbstate_t *restrict ps);
size_t mbsrtowcs(wchar_t *restrict dst, const char **restrict src, size_t len,
                 mbstate_t *restrict ps);
size_t wcsrtombs(char *restrict dst, const wchar_t **restrict src, size_t len,
                 mbstate_t *restrict ps);

int fwide(FILE *stream, int mode);
wint_t fgetwc(FILE *stream);
wint_t getwc(FILE *stream);
wint_t getwchar(void);
wchar_t *fgetws(wchar_t *restrict s, int n, FILE *restrict stream);
wint_t fputwc(wchar_t wc, FILE *stream);
wint_t putwc(wchar_t wc, FILE *stream);
wint_t putwchar(wchar_t wc);
int fputws(const wchar_t *restrict s, FILE *restrict stream);
wint_t ungetwc(wint_t wc, FILE *stream);

size_t wcslen(const wchar_t *s);
int wcscmp(const wchar_t *left, const wchar_t *right);
wchar_t *wcscpy(wchar_t *restrict dst, const wchar_t *restrict src);

#endif
