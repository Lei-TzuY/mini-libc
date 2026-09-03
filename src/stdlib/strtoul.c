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

unsigned long strtoul(const char *restrict nptr, char **restrict endptr, int base)
{
    const unsigned char *s = (const unsigned char *)nptr;
    const unsigned long limit = ~0UL;
    unsigned long value = 0;
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
        return limit;
    }
    if (negative) {
        return 0UL - value;
    }
    return value;
}
