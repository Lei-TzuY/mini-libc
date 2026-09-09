#include <errno.h>
#include <stddef.h>
#include <wchar.h>

#define MINI_C_MB_MAX 0x7fU

static void reset_state(mbstate_t *ps)
{
    if (ps != (mbstate_t *)0) {
        ps->__count = 0U;
        ps->__value = 0U;
    }
}

static int decode_byte(unsigned char byte, wchar_t *out)
{
    if (byte > MINI_C_MB_MAX) {
        errno = EILSEQ;
        return -1;
    }
    if (out != (wchar_t *)0) {
        *out = (wchar_t)byte;
    }
    return byte == 0U ? 0 : 1;
}

static int encode_wide(wchar_t wc, char *out)
{
    if (wc < 0 || (unsigned int)wc > MINI_C_MB_MAX) {
        errno = EILSEQ;
        return -1;
    }
    if (out != (char *)0) {
        *out = (char)wc;
    }
    return 1;
}

int mbsinit(const mbstate_t *ps)
{
    return ps == (const mbstate_t *)0 ||
           (ps->__count == 0U && ps->__value == 0U);
}

size_t mbrtowc(wchar_t *restrict pwc, const char *restrict s, size_t n,
               mbstate_t *restrict ps)
{
    int decoded;

    if (s == (const char *)0) {
        reset_state(ps);
        return 0U;
    }
    if (n == 0U) {
        reset_state(ps);
        return (size_t)-2;
    }

    decoded = decode_byte((unsigned char)*s, pwc);
    reset_state(ps);
    if (decoded < 0) {
        return (size_t)-1;
    }
    return (size_t)decoded;
}

size_t wcrtomb(char *restrict s, wchar_t wc, mbstate_t *restrict ps)
{
    int encoded;

    if (s == (char *)0) {
        reset_state(ps);
        return 1U;
    }

    encoded = encode_wide(wc, s);
    reset_state(ps);
    if (encoded < 0) {
        return (size_t)-1;
    }
    return (size_t)encoded;
}

size_t mbsrtowcs(wchar_t *restrict dst, const char **restrict src, size_t len,
                 mbstate_t *restrict ps)
{
    const char *cursor = *src;
    size_t count = 0U;

    if (dst == (wchar_t *)0) {
        for (;;) {
            size_t converted = mbrtowc((wchar_t *)0, cursor, 1U, ps);

            if (converted == (size_t)-1) {
                return (size_t)-1;
            }
            if (converted == 0U) {
                return count;
            }
            cursor += converted;
            ++count;
        }
    }

    while (count < len) {
        wchar_t wc = 0;
        size_t converted = mbrtowc(&wc, cursor, 1U, ps);

        if (converted == (size_t)-1) {
            *src = cursor;
            return (size_t)-1;
        }
        dst[count] = wc;
        if (converted == 0U) {
            *src = (const char *)0;
            return count;
        }
        cursor += converted;
        ++count;
    }

    *src = cursor;
    return count;
}

size_t wcsrtombs(char *restrict dst, const wchar_t **restrict src, size_t len,
                 mbstate_t *restrict ps)
{
    const wchar_t *cursor = *src;
    size_t count = 0U;

    if (dst == (char *)0) {
        while (*cursor != 0) {
            char byte;
            size_t converted = wcrtomb(&byte, *cursor, ps);

            if (converted == (size_t)-1) {
                return (size_t)-1;
            }
            count += converted;
            ++cursor;
        }
        return count;
    }

    while (count < len) {
        char byte;
        size_t converted = wcrtomb(&byte, *cursor, ps);

        if (converted == (size_t)-1) {
            *src = cursor;
            return (size_t)-1;
        }
        if (converted > len - count) {
            *src = cursor;
            return count;
        }
        dst[count] = byte;
        if (*cursor == 0) {
            *src = (const wchar_t *)0;
            return count;
        }
        count += converted;
        ++cursor;
    }

    *src = cursor;
    return count;
}

size_t wcslen(const wchar_t *s)
{
    const wchar_t *cursor = s;

    while (*cursor != 0) {
        ++cursor;
    }
    return (size_t)(cursor - s);
}

int wcscmp(const wchar_t *left, const wchar_t *right)
{
    while (*left != 0 && *left == *right) {
        ++left;
        ++right;
    }
    if (*left < *right) {
        return -1;
    }
    if (*left > *right) {
        return 1;
    }
    return 0;
}

wchar_t *wcscpy(wchar_t *restrict dst, const wchar_t *restrict src)
{
    wchar_t *result = dst;

    do {
        *dst = *src;
        ++dst;
        ++src;
    } while (src[-1] != 0);

    return result;
}
