#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

unsigned long mini_test_strtoul(const char *nptr, char **endptr, int base);
int *__mini_errno_location(void);

static unsigned int rng_state = 0x7f4a7c15U;

static unsigned int next_random(void)
{
    rng_state = rng_state * 1664525U + 1013904223U;
    return rng_state;
}

static int compare_case(const char *text, int base)
{
    char host_sentinel;
    char mini_sentinel;
    char *host_end = &host_sentinel;
    char *mini_end = &mini_sentinel;
    int *mini_errno = __mini_errno_location();
    unsigned long host_value;
    unsigned long mini_value;
    long host_offset = -1;
    long mini_offset = -1;

    errno = 7;
    *mini_errno = 7;

    host_value = strtoul(text, &host_end, base);
    mini_value = mini_test_strtoul(text, &mini_end, base);

    if (host_end != &host_sentinel) {
        host_offset = (long)(host_end - text);
    }
    if (mini_end != &mini_sentinel) {
        mini_offset = (long)(mini_end - text);
    }

    if (host_value != mini_value || host_offset != mini_offset ||
        errno != *mini_errno) {
        fprintf(stderr,
                "mismatch: text=[%s] base=%d host=(%lu,%ld,%d) mini=(%lu,%ld,%d)\n",
                text, base, host_value, host_offset, errno,
                mini_value, mini_offset, *mini_errno);
        return 0;
    }
    return 1;
}

int main(void)
{
    static const char *const cases[] = {
        "", " ", "+", "-", "42", "  -42tail", "077", "09",
        "0x", "0xg", "0x1fZ", "-0X2A!", "10102", "zZ",
        "18446744073709551615", "18446744073709551616",
        "-1", "-18446744073709551615", "-18446744073709551616",
        "9999999999999999999999999999999999999999999",
        "-9999999999999999999999999999999999999999999"
    };
    static const int bases[] = {1, 37, 0, 2, 3, 8, 10, 16, 20, 36};
    static const char alphabet[] = " \t\n\v\f\r+-0xX123456789abcdefABCDEFghizZ!";
    char text[48];
    size_t i;
    size_t j;

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        for (j = 0; j < sizeof(bases) / sizeof(bases[0]); ++j) {
            if (!compare_case(cases[i], bases[j])) {
                return 1;
            }
        }
    }

    for (i = 0; i < 5000; ++i) {
        size_t length = (size_t)(next_random() % 40U);
        int base = bases[next_random() % (sizeof(bases) / sizeof(bases[0]))];

        for (j = 0; j < length; ++j) {
            text[j] = alphabet[next_random() % (sizeof(alphabet) - 1U)];
        }
        text[length] = '\0';

        if (!compare_case(text, base)) {
            return 1;
        }
    }

    puts("strtoul differential passed");
    return 0;
}
