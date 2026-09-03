#include <stdlib.h>

static int is_c_space(unsigned char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' ||
           c == '\f' || c == '\r';
}

int atoi(const char *nptr)
{
    const unsigned char *s = (const unsigned char *)nptr;
    unsigned int limit = (unsigned int)__INT_MAX__;
    unsigned int value = 0;
    int negative = 0;
    int overflow = 0;

    while (is_c_space(*s)) {
        ++s;
    }

    if (*s == '+' || *s == '-') {
        negative = *s == '-';
        ++s;
    }

    if (negative) {
        limit += 1U;
    }

    while (*s >= '0' && *s <= '9') {
        unsigned int digit = (unsigned int)(*s - '0');

        if (!overflow) {
            if (value > (limit - digit) / 10U) {
                overflow = 1;
            } else {
                value = value * 10U + digit;
            }
        }
        ++s;
    }

    if (overflow) {
        return negative ? -__INT_MAX__ - 1 : __INT_MAX__;
    }
    if (negative) {
        if (value == (unsigned int)__INT_MAX__ + 1U) {
            return -__INT_MAX__ - 1;
        }
        return -(int)value;
    }
    return (int)value;
}
