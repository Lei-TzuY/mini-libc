#include <errno.h>
#include <stddef.h>
#include <stdlib.h>

#define MINI_C_MAX 0x7fU

static int decode(unsigned char byte, wchar_t *out)
{
    if (byte > MINI_C_MAX) {
        errno = EILSEQ;
        return -1;
    }
    if (out != (wchar_t *)0) {
        *out = (wchar_t)byte;
    }
    return byte == 0U ? 0 : 1;
}

int mblen(const char *s, size_t n)
{
    if (s == (const char *)0) return 0;
    if (n == 0U) return -1;
    return decode((unsigned char)*s, (wchar_t *)0);
}

int mbtowc(wchar_t *restrict pwc, const char *restrict s, size_t n)
{
    if (s == (const char *)0) return 0;
    if (n == 0U) return -1;
    return decode((unsigned char)*s, pwc);
}

int wctomb(char *s, wchar_t wc)
{
    if (s == (char *)0) return 0;
    if (wc < 0 || (unsigned int)wc > MINI_C_MAX) {
        errno = EILSEQ;
        return -1;
    }
    *s = (char)wc;
    return 1;
}

size_t mbstowcs(wchar_t *restrict dst, const char *restrict src, size_t len)
{
    size_t count = 0U;
    if (dst == (wchar_t *)0) {
        while (src[count] != '\0') {
            if ((unsigned char)src[count] > MINI_C_MAX) {
                errno = EILSEQ;
                return (size_t)-1;
            }
            ++count;
        }
        return count;
    }
    while (count < len) {
        unsigned char byte = (unsigned char)src[count];
        if (byte > MINI_C_MAX) {
            errno = EILSEQ;
            return (size_t)-1;
        }
        dst[count] = (wchar_t)byte;
        if (byte == 0U) return count;
        ++count;
    }
    return count;
}

size_t wcstombs(char *restrict dst, const wchar_t *restrict src, size_t len)
{
    size_t count = 0U;
    if (dst == (char *)0) {
        while (src[count] != 0) {
            wchar_t wc = src[count];
            if (wc < 0 || (unsigned int)wc > MINI_C_MAX) {
                errno = EILSEQ;
                return (size_t)-1;
            }
            ++count;
        }
        return count;
    }
    while (count < len) {
        wchar_t wc = src[count];
        if (wc < 0 || (unsigned int)wc > MINI_C_MAX) {
            errno = EILSEQ;
            return (size_t)-1;
        }
        dst[count] = (char)wc;
        if (wc == 0) return count;
        ++count;
    }
    return count;
}
