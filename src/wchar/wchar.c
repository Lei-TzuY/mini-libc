#include <errno.h>
#include <stddef.h>
#include <wchar.h>

#include "../locale/locale_internal.h"
#include "wchar_internal.h"

#define MINI_C_MB_MAX 0x7fU
#define MINI_UTF8_MAX 0x10ffffU
#define MINI_UTF8_SURROGATE_FIRST 0xd800U
#define MINI_UTF8_SURROGATE_LAST 0xdfffU
#define MINI_STATE_REMAIN_MASK 0xffU
#define MINI_STATE_TOTAL_SHIFT 8U

static mbstate_t mini_mbr_state = {0U, 0U};
static mbstate_t mini_wcr_state = {0U, 0U};

static void reset_state(mbstate_t *ps)
{
    if (ps != (mbstate_t *)0) {
        ps->__count = 0U;
        ps->__value = 0U;
    }
}

static mbstate_t *select_state(mbstate_t *ps, mbstate_t *fallback)
{
    return ps != (mbstate_t *)0 ? ps : fallback;
}

static unsigned int state_remaining(const mbstate_t *ps)
{
    return ps->__count & MINI_STATE_REMAIN_MASK;
}

static unsigned int state_total(const mbstate_t *ps)
{
    return ps->__count >> MINI_STATE_TOTAL_SHIFT;
}

static void start_state(mbstate_t *ps, unsigned int total,
                        unsigned int remaining, unsigned int value)
{
    ps->__count = (total << MINI_STATE_TOTAL_SHIFT) | remaining;
    ps->__value = value;
}

static int valid_scalar(unsigned int value)
{
    return value <= MINI_UTF8_MAX &&
           (value < MINI_UTF8_SURROGATE_FIRST ||
            value > MINI_UTF8_SURROGATE_LAST);
}

static unsigned int minimum_scalar(unsigned int total)
{
    if (total == 2U) {
        return 0x80U;
    }
    if (total == 3U) {
        return 0x800U;
    }
    return 0x10000U;
}

int mbsinit(const mbstate_t *ps)
{
    return ps == (const mbstate_t *)0 ||
           (ps->__count == 0U && ps->__value == 0U);
}

size_t __mini_mbrtowc_mode(wchar_t *restrict pwc, const char *restrict s,
                           size_t n, mbstate_t *restrict ps, int utf8)
{
    mbstate_t *state = select_state(ps, &mini_mbr_state);
    size_t used = 0U;

    if (s == (const char *)0) {
        reset_state(state);
        return 0U;
    }
    if (n == 0U) {
        return (size_t)-2;
    }

    if (!utf8) {
        unsigned char byte;

        if (!mbsinit(state)) {
            reset_state(state);
            errno = EILSEQ;
            return (size_t)-1;
        }
        byte = (unsigned char)s[0];
        if (byte > MINI_C_MB_MAX) {
            errno = EILSEQ;
            return (size_t)-1;
        }
        if (pwc != (wchar_t *)0) {
            *pwc = (wchar_t)byte;
        }
        return byte == 0U ? 0U : 1U;
    }

    while (used < n) {
        unsigned char byte = (unsigned char)s[used];

        if (state_remaining(state) == 0U) {
            if (byte == 0U) {
                if (pwc != (wchar_t *)0) {
                    *pwc = 0;
                }
                reset_state(state);
                return 0U;
            }
            if (byte <= 0x7fU) {
                if (pwc != (wchar_t *)0) {
                    *pwc = (wchar_t)byte;
                }
                return 1U;
            }
            if (byte >= 0xc2U && byte <= 0xdfU) {
                start_state(state, 2U, 1U, (unsigned int)(byte & 0x1fU));
            } else if (byte >= 0xe0U && byte <= 0xefU) {
                start_state(state, 3U, 2U, (unsigned int)(byte & 0x0fU));
            } else if (byte >= 0xf0U && byte <= 0xf4U) {
                start_state(state, 4U, 3U, (unsigned int)(byte & 0x07U));
            } else {
                reset_state(state);
                errno = EILSEQ;
                return (size_t)-1;
            }
            ++used;
        } else {
            unsigned int remaining;
            unsigned int total;
            unsigned int value;

            if (byte < 0x80U || byte > 0xbfU) {
                reset_state(state);
                errno = EILSEQ;
                return (size_t)-1;
            }

            remaining = state_remaining(state) - 1U;
            total = state_total(state);
            value = (state->__value << 6) | (unsigned int)(byte & 0x3fU);
            ++used;

            if (remaining != 0U) {
                start_state(state, total, remaining, value);
            } else {
                unsigned int minimum = minimum_scalar(total);

                reset_state(state);
                if (value < minimum || !valid_scalar(value)) {
                    errno = EILSEQ;
                    return (size_t)-1;
                }
                if (pwc != (wchar_t *)0) {
                    *pwc = (wchar_t)value;
                }
                return used;
            }
        }
    }

    return (size_t)-2;
}

size_t mbrtowc(wchar_t *restrict pwc, const char *restrict s, size_t n,
               mbstate_t *restrict ps)
{
    return __mini_mbrtowc_mode(pwc, s, n, ps, __mini_locale_is_utf8());
}

size_t __mini_wcrtomb_mode(char *restrict s, wchar_t wc,
                           mbstate_t *restrict ps, int utf8)
{
    mbstate_t *state = select_state(ps, &mini_wcr_state);
    unsigned int value;

    if (s == (char *)0) {
        reset_state(state);
        return 1U;
    }

    reset_state(state);
    if (wc < 0) {
        errno = EILSEQ;
        return (size_t)-1;
    }
    value = (unsigned int)wc;

    if (!utf8) {
        if (value > MINI_C_MB_MAX) {
            errno = EILSEQ;
            return (size_t)-1;
        }
        s[0] = (char)value;
        return 1U;
    }

    if (!valid_scalar(value)) {
        errno = EILSEQ;
        return (size_t)-1;
    }
    if (value <= 0x7fU) {
        s[0] = (char)value;
        return 1U;
    }
    if (value <= 0x7ffU) {
        s[0] = (char)(0xc0U | (value >> 6));
        s[1] = (char)(0x80U | (value & 0x3fU));
        return 2U;
    }
    if (value <= 0xffffU) {
        s[0] = (char)(0xe0U | (value >> 12));
        s[1] = (char)(0x80U | ((value >> 6) & 0x3fU));
        s[2] = (char)(0x80U | (value & 0x3fU));
        return 3U;
    }

    s[0] = (char)(0xf0U | (value >> 18));
    s[1] = (char)(0x80U | ((value >> 12) & 0x3fU));
    s[2] = (char)(0x80U | ((value >> 6) & 0x3fU));
    s[3] = (char)(0x80U | (value & 0x3fU));
    return 4U;
}

size_t wcrtomb(char *restrict s, wchar_t wc, mbstate_t *restrict ps)
{
    return __mini_wcrtomb_mode(s, wc, ps, __mini_locale_is_utf8());
}

static size_t source_window(const char *s, size_t maximum)
{
    size_t length = 0U;

    while (length < maximum) {
        ++length;
        if (s[length - 1U] == '\0') {
            break;
        }
    }
    return length;
}

size_t mbsrtowcs(wchar_t *restrict dst, const char **restrict src, size_t len,
                 mbstate_t *restrict ps)
{
    const char *cursor = *src;
    size_t count = 0U;
    mbstate_t sizing_state;
    mbstate_t *state = ps;
    int sizing = dst == (wchar_t *)0;

    if (sizing) {
        if (ps != (mbstate_t *)0) {
            sizing_state = *ps;
        } else {
            sizing_state.__count = 0U;
            sizing_state.__value = 0U;
        }
        state = &sizing_state;
    }

    for (;;) {
        wchar_t wc = 0;
        size_t converted;
        size_t window;

        if (!sizing && count >= len) {
            *src = cursor;
            return count;
        }

        window = source_window(cursor, __mini_locale_is_utf8() ? 4U : 1U);
        converted = mbrtowc(&wc, cursor, window, state);
        if (converted == (size_t)-1 || converted == (size_t)-2) {
            if (!sizing) {
                *src = cursor;
            }
            if (converted == (size_t)-2) {
                errno = EILSEQ;
            }
            return (size_t)-1;
        }
        if (converted == 0U) {
            if (!sizing) {
                dst[count] = 0;
                *src = (const char *)0;
            }
            return count;
        }

        if (!sizing) {
            dst[count] = wc;
        }
        ++count;
        cursor += converted;
    }
}

size_t wcsrtombs(char *restrict dst, const wchar_t **restrict src, size_t len,
                 mbstate_t *restrict ps)
{
    const wchar_t *cursor = *src;
    size_t count = 0U;
    mbstate_t sizing_state;
    mbstate_t *state = ps;
    int sizing = dst == (char *)0;

    if (sizing) {
        if (ps != (mbstate_t *)0) {
            sizing_state = *ps;
        } else {
            sizing_state.__count = 0U;
            sizing_state.__value = 0U;
        }
        state = &sizing_state;
    }

    for (;;) {
        char encoded[4];
        size_t converted = wcrtomb(encoded, *cursor, state);
        size_t i;

        if (converted == (size_t)-1) {
            if (!sizing) {
                *src = cursor;
            }
            return (size_t)-1;
        }
        if (*cursor == 0) {
            if (!sizing) {
                if (count >= len) {
                    *src = cursor;
                    return count;
                }
                dst[count] = '\0';
                *src = (const wchar_t *)0;
            }
            return count;
        }
        if (!sizing && converted > len - count) {
            *src = cursor;
            return count;
        }

        if (!sizing) {
            for (i = 0U; i < converted; ++i) {
                dst[count + i] = encoded[i];
            }
        }
        count += converted;
        ++cursor;
    }
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
