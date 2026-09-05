#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

float mini_test_strtof(const char *nptr, char **endptr);
double mini_test_strtod(const char *nptr, char **endptr);
int *__mini_errno_location(void);

static unsigned long long double_bits(double value)
{
    unsigned long long bits = 0ULL;
    unsigned char *out = (unsigned char *)&bits;
    const unsigned char *in = (const unsigned char *)&value;
    size_t i;

    for (i = 0; i < 8U; ++i) {
        out[i] = in[i];
    }
    return bits;
}

static unsigned int float_bits(float value)
{
    unsigned int bits = 0U;
    unsigned char *out = (unsigned char *)&bits;
    const unsigned char *in = (const unsigned char *)&value;
    size_t i;

    for (i = 0; i < 4U; ++i) {
        out[i] = in[i];
    }
    return bits;
}

static int double_nan(double value)
{
    unsigned long long bits = double_bits(value);

    return ((bits >> 52) & 0x7ffULL) == 0x7ffULL &&
           (bits & 0xfffffffffffffULL) != 0ULL;
}

static int float_nan(float value)
{
    unsigned int bits = float_bits(value);

    return ((bits >> 23) & 0xffU) == 0xffU &&
           (bits & 0x7fffffU) != 0U;
}

static int compare_double(const char *text)
{
    char *host_end;
    char *mini_end;
    int *mini_errno = __mini_errno_location();
    double host_value;
    double mini_value;
    long host_offset;
    long mini_offset;

    errno = 7;
    *mini_errno = 7;
    host_value = strtod(text, &host_end);
    mini_value = mini_test_strtod(text, &mini_end);
    host_offset = (long)(host_end - text);
    mini_offset = (long)(mini_end - text);

    if (host_offset != mini_offset || errno != *mini_errno) {
        fprintf(stderr,
                "strtod lexical mismatch: [%s] host=(%ld,%d) mini=(%ld,%d)\n",
                text, host_offset, errno, mini_offset, *mini_errno);
        return 0;
    }
    if (double_nan(host_value) || double_nan(mini_value)) {
        if (!double_nan(host_value) || !double_nan(mini_value) ||
            ((double_bits(host_value) ^ double_bits(mini_value)) >> 63) != 0ULL) {
            fprintf(stderr, "strtod NaN mismatch: [%s]\n", text);
            return 0;
        }
        return 1;
    }
    if (double_bits(host_value) != double_bits(mini_value)) {
        fprintf(stderr,
                "strtod value mismatch: [%s] host=0x%llx mini=0x%llx\n",
                text, double_bits(host_value), double_bits(mini_value));
        return 0;
    }
    return 1;
}

static int compare_float(const char *text)
{
    char *host_end;
    char *mini_end;
    int *mini_errno = __mini_errno_location();
    float host_value;
    float mini_value;
    long host_offset;
    long mini_offset;

    errno = 7;
    *mini_errno = 7;
    host_value = strtof(text, &host_end);
    mini_value = mini_test_strtof(text, &mini_end);
    host_offset = (long)(host_end - text);
    mini_offset = (long)(mini_end - text);

    if (host_offset != mini_offset || errno != *mini_errno) {
        fprintf(stderr,
                "strtof lexical mismatch: [%s] host=(%ld,%d) mini=(%ld,%d)\n",
                text, host_offset, errno, mini_offset, *mini_errno);
        return 0;
    }
    if (float_nan(host_value) || float_nan(mini_value)) {
        if (!float_nan(host_value) || !float_nan(mini_value) ||
            ((float_bits(host_value) ^ float_bits(mini_value)) >> 31) != 0U) {
            fprintf(stderr, "strtof NaN mismatch: [%s]\n", text);
            return 0;
        }
        return 1;
    }
    if (float_bits(host_value) != float_bits(mini_value)) {
        fprintf(stderr,
                "strtof value mismatch: [%s] host=0x%x mini=0x%x\n",
                text, float_bits(host_value), float_bits(mini_value));
        return 0;
    }
    return 1;
}

int main(void)
{
    static const char *const double_cases[] = {
        "", " ", "+", "-", "0", "-0", "-0.0x", "1", "-1.5",
        ".75x", "1.x", "1e2x", "1e+X", "0xg", "0x1p0x",
        "0x1.8p1x", "0x1fp0x", "0x1fZ", "INFx", "infinityX",
        "infiX", "-INFx", "nanX", "nan(tag)X", "nan(tagX",
        "1e9999", "1e-9999", "0e999999", "0x0p999999"
    };
    static const char *const float_cases[] = {
        "", "+X", "0", "-0.0x", "1", "-1.5", ".5x", ".25x",
        ".125x", "3.5x", "0x1p0x", "0x1p2x", "0x1.8p1x",
        "INFx", "-INFINITYx", "nanX", "nan(tag)X",
        "1e999", "1e-50", "0e999999"
    };
    size_t i;

    for (i = 0; i < sizeof(double_cases) / sizeof(double_cases[0]); ++i) {
        if (!compare_double(double_cases[i])) {
            return 1;
        }
    }
    for (i = 0; i < sizeof(float_cases) / sizeof(float_cases[0]); ++i) {
        if (!compare_float(float_cases[i])) {
            return 2;
        }
    }

    puts("strtod differential passed");
    return 0;
}
