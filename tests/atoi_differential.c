#include <stdlib.h>

#define RANDOM_CASES 5000
#define BUF_SIZE 64

int mini_test_atoi(const char *nptr);

static unsigned long rng_state = 0xa0761d6478bd642fUL;

static unsigned long next_random(void)
{
    rng_state ^= rng_state << 7;
    rng_state ^= rng_state >> 9;
    rng_state ^= rng_state << 8;
    return rng_state;
}

static char *append_unsigned(char *out, unsigned int value)
{
    char reversed[16];
    unsigned int count = 0;

    do {
        reversed[count++] = (char)('0' + value % 10U);
        value /= 10U;
    } while (value != 0U);

    while (count != 0U) {
        *out++ = reversed[--count];
    }
    return out;
}

static void build_case(char *buf, int value, unsigned long style)
{
    static const char whitespace[] = {' ', '\t', '\n', '\v', '\f', '\r'};
    char *out = buf;
    unsigned int magnitude;
    unsigned int spaces = (unsigned int)(style % 7UL);
    unsigned int i;

    for (i = 0; i < spaces; ++i) {
        *out++ = whitespace[(style >> (i * 3U)) % 6UL];
    }

    if (value < 0) {
        *out++ = '-';
        magnitude = (unsigned int)(-(value + 1)) + 1U;
    } else {
        if ((style & 0x40UL) != 0UL) {
            *out++ = '+';
        }
        magnitude = (unsigned int)value;
    }

    out = append_unsigned(out, magnitude);
    if ((style & 0x80UL) != 0UL) {
        *out++ = 'x';
        *out++ = '9';
    }
    *out = '\0';
}

int main(void)
{
    char buf[BUF_SIZE];
    int case_no;

    for (case_no = 0; case_no < RANDOM_CASES; ++case_no) {
        unsigned long random = next_random();
        int value = (int)(random % 2000000001UL) - 1000000000;

        build_case(buf, value, next_random());
        if (mini_test_atoi(buf) != atoi(buf)) {
            return 1;
        }
    }

    return 0;
}
