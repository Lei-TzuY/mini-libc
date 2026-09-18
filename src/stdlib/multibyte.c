#include <stddef.h>
#include <stdlib.h>
#include <wchar.h>

int mblen(const char *s, size_t n)
{
    mbstate_t state = {0U, 0U};
    size_t result;

    if (s == (const char *)0) {
        return 0;
    }
    result = mbrtowc((wchar_t *)0, s, n, &state);
    if (result == (size_t)-1 || result == (size_t)-2) {
        return -1;
    }
    return (int)result;
}

int mbtowc(wchar_t *restrict pwc, const char *restrict s, size_t n)
{
    mbstate_t state = {0U, 0U};
    size_t result;

    if (s == (const char *)0) {
        return 0;
    }
    result = mbrtowc(pwc, s, n, &state);
    if (result == (size_t)-1 || result == (size_t)-2) {
        return -1;
    }
    return (int)result;
}

int wctomb(char *s, wchar_t wc)
{
    mbstate_t state = {0U, 0U};
    size_t result;

    if (s == (char *)0) {
        return 0;
    }
    result = wcrtomb(s, wc, &state);
    return result == (size_t)-1 ? -1 : (int)result;
}

size_t mbstowcs(wchar_t *restrict dst, const char *restrict src, size_t len)
{
    const char *cursor = src;
    mbstate_t state = {0U, 0U};

    return mbsrtowcs(dst, &cursor, len, &state);
}

size_t wcstombs(char *restrict dst, const wchar_t *restrict src, size_t len)
{
    const wchar_t *cursor = src;
    mbstate_t state = {0U, 0U};

    return wcsrtombs(dst, &cursor, len, &state);
}
