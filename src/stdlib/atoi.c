#include <stdlib.h>

static int is_c_space(unsigned char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' ||
           c == '\f' || c == '\r';
}

int atoi(const char *nptr)
{
    const unsigned char *s = (const unsigned char *)nptr;
    const unsigned int int_max = ~0U >> 1;
    unsigned int limit = int_max;
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
        return negative ? -(int)int_max - 1 : (int)int_max;
    }
    if (negative) {
        if (value == int_max + 1U) {
            return -(int)int_max - 1;
        }
        return -(int)value;
    }
    return (int)value;
}
