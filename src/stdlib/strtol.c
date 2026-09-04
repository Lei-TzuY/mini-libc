#include <errno.h>
#include <stdlib.h>

static int is_c_space(unsigned char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' ||
           c == '\f' || c == '\r';
}

static int digit_value(unsigned char c)
{
    if (c >= '0' && c <= '9') {
        return (int)(c - '0');
    }
    if (c >= 'a' && c <= 'z') {
        return (int)(c - 'a') + 10;
    }
    if (c >= 'A' && c <= 'Z') {
        return (int)(c - 'A') + 10;
    }
    return -1;
}

long strtol(const char *restrict nptr, char **restrict endptr, int base)
{
    const unsigned char *s = (const unsigned char *)nptr;
    const unsigned long long_max = ~0UL >> 1;
    unsigned long value = 0;
    unsigned long limit;
    int negative = 0;
    int any = 0;
    int overflow = 0;

    if (base != 0 && (base < 2 || base > 36)) {
        errno = EINVAL;
        return 0;
    }

    while (is_c_space(*s)) {
        ++s;
    }

    if (*s == '+' || *s == '-') {
        negative = *s == '-';
        ++s;
    }

    if (base == 0) {
        if (s[0] == '0') {
            if (s[1] == 'x' || s[1] == 'X') {
                int next = digit_value(s[2]);

                if (next >= 0 && next < 16) {
                    base = 16;
                    s += 2;
                } else {
                    base = 8;
                }
            } else {
                base = 8;
            }
        } else {
            base = 10;
        }
    } else if (base == 16 && s[0] == '0' &&
               (s[1] == 'x' || s[1] == 'X')) {
        int next = digit_value(s[2]);

        if (next >= 0 && next < 16) {
            s += 2;
        }
    }

    limit = long_max;
    if (negative) {
        limit += 1UL;
    }

    for (;;) {
        int digit = digit_value(*s);

        if (digit < 0 || digit >= base) {
            break;
        }
        any = 1;
        if (!overflow) {
            unsigned long udigit = (unsigned long)digit;
            unsigned long ubase = (unsigned long)base;

            if (value > (limit - udigit) / ubase) {
                overflow = 1;
            } else {
                value = value * ubase + udigit;
            }
        }
        ++s;
    }

    if (!any) {
        if (endptr != (char **)0) {
            *endptr = (char *)nptr;
        }
        return 0;
    }

    if (endptr != (char **)0) {
        *endptr = (char *)s;
    }

    if (overflow) {
        errno = ERANGE;
        return negative ? -(long)long_max - 1L : (long)long_max;
    }
    if (negative) {
        if (value == long_max + 1UL) {
            return -(long)long_max - 1L;
        }
        return -(long)value;
    }
    return (long)value;
}

long long strtoll(const char *restrict nptr, char **restrict endptr, int base)
{
    const unsigned char *s = (const unsigned char *)nptr;
    const unsigned long long long_long_max = ~0ULL >> 1;
    unsigned long long value = 0;
    unsigned long long limit;
    int negative = 0;
    int any = 0;
    int overflow = 0;

    if (base != 0 && (base < 2 || base > 36)) {
        errno = EINVAL;
        return 0;
    }

    while (is_c_space(*s)) {
        ++s;
    }

    if (*s == '+' || *s == '-') {
        negative = *s == '-';
        ++s;
    }

    if (base == 0) {
        if (s[0] == '0') {
            if (s[1] == 'x' || s[1] == 'X') {
                int next = digit_value(s[2]);

                if (next >= 0 && next < 16) {
                    base = 16;
                    s += 2;
                } else {
                    base = 8;
                }
            } else {
                base = 8;
            }
        } else {
            base = 10;
        }
    } else if (base == 16 && s[0] == '0' &&
               (s[1] == 'x' || s[1] == 'X')) {
        int next = digit_value(s[2]);

        if (next >= 0 && next < 16) {
            s += 2;
        }
    }

    limit = long_long_max;
    if (negative) {
        limit += 1ULL;
    }

    for (;;) {
        int digit = digit_value(*s);

        if (digit < 0 || digit >= base) {
            break;
        }
        any = 1;
        if (!overflow) {
            unsigned long long udigit = (unsigned long long)digit;
            unsigned long long ubase = (unsigned long long)base;

            if (value > (limit - udigit) / ubase) {
                overflow = 1;
            } else {
                value = value * ubase + udigit;
            }
        }
        ++s;
    }

    if (!any) {
        if (endptr != (char **)0) {
            *endptr = (char *)nptr;
        }
        return 0;
    }

    if (endptr != (char **)0) {
        *endptr = (char *)s;
    }

    if (overflow) {
        errno = ERANGE;
        return negative ? -(long long)long_long_max - 1LL :
                          (long long)long_long_max;
    }
    if (negative) {
        if (value == long_long_max + 1ULL) {
            return -(long long)long_long_max - 1LL;
        }
        return -(long long)value;
    }
    return (long long)value;
}
