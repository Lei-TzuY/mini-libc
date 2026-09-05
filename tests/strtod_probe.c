#include <errno.h>
#include <mini/syscall.h>
#include <stdio.h>
#include <stdlib.h>

static unsigned long long double_bits(double value)
{
    unsigned long long bits = 0ULL;
    unsigned char *out = (unsigned char *)&bits;
    const unsigned char *in = (const unsigned char *)&value;
    unsigned int i;

    for (i = 0U; i < 8U; ++i) {
        out[i] = in[i];
    }
    return bits;
}

static unsigned int float_bits(float value)
{
    unsigned int bits = 0U;
    unsigned char *out = (unsigned char *)&bits;
    const unsigned char *in = (const unsigned char *)&value;
    unsigned int i;

    for (i = 0U; i < 4U; ++i) {
        out[i] = in[i];
    }
    return bits;
}

static int is_double_infinity(double value, int negative)
{
    unsigned long long bits = double_bits(value);
    unsigned long long expected = 0x7ff0000000000000ULL;

    if (negative) {
        expected |= 0x8000000000000000ULL;
    }
    return bits == expected;
}

static int is_double_nan(double value)
{
    unsigned long long bits = double_bits(value);

    return ((bits >> 52) & 0x7ffULL) == 0x7ffULL &&
           (bits & 0xfffffffffffffULL) != 0ULL;
}

static int is_float_infinity(float value, int negative)
{
    unsigned int bits = float_bits(value);
    unsigned int expected = 0x7f800000U;

    if (negative) {
        expected |= 0x80000000U;
    }
    return bits == expected;
}

int main(void)
{
    static const char malformed_exp[] = "1e+X";
    static const char malformed_hex[] = "0xg";
    static const char malformed_nan[] = "nan(tagX";
    static const char ok[] = "strtod-ok\n";
    char *end;
    double value;
    double first;
    double second;
    double third;
    float single;

    errno = EIO;
    value = strtod("  -1.5e2tail", &end);
    if (value != -150.0 || *end != 't' || errno != EIO) {
        return 1;
    }
    value = strtod(".75X", &end);
    if (value != 0.75 || *end != 'X' || errno != EIO) {
        return 2;
    }
    value = strtod("1.X", &end);
    if (value != 1.0 || *end != 'X' || errno != EIO) {
        return 3;
    }

    end = (char *)0;
    value = strtod("+X", &end);
    if (value != 0.0 || end == (char *)0 || end[0] != '+' || errno != EIO) {
        return 4;
    }
    value = strtod(malformed_exp, &end);
    if (value != 1.0 || end != malformed_exp + 1 || errno != EIO) {
        return 5;
    }
    value = strtod(malformed_hex, &end);
    if (value != 0.0 || end != malformed_hex + 1 || errno != EIO) {
        return 6;
    }

    value = strtod("0x1.8p1tail", &end);
    if (value != 3.0 || *end != 't' || errno != EIO) {
        return 7;
    }
    value = strtod("0x1fZ", &end);
    if (value != 31.0 || *end != 'Z' || errno != EIO) {
        return 8;
    }
    value = strtod("-INFINITY!", &end);
    if (!is_double_infinity(value, 1) || *end != '!' || errno != EIO) {
        return 9;
    }
    value = strtod("nan(tag)X", &end);
    if (!is_double_nan(value) || *end != 'X' || errno != EIO) {
        return 10;
    }
    value = strtod(malformed_nan, &end);
    if (!is_double_nan(value) || end != malformed_nan + 3 || errno != EIO) {
        return 11;
    }
    value = strtod("-0.0X", &end);
    if (double_bits(value) != 0x8000000000000000ULL || *end != 'X' ||
        errno != EIO) {
        return 12;
    }

    errno = EIO;
    value = strtod("1e9999", &end);
    if (!is_double_infinity(value, 0) || *end != '\0' || errno != ERANGE) {
        return 13;
    }
    errno = EIO;
    value = strtod("1e-9999", &end);
    if (double_bits(value) != 0ULL || *end != '\0' || errno != ERANGE) {
        return 14;
    }
    errno = EIO;
    value = strtod("0e999999", &end);
    if (double_bits(value) != 0ULL || *end != '\0' || errno != EIO) {
        return 15;
    }

    errno = EIO;
    single = strtof("3.5X", &end);
    if (single != 3.5f || *end != 'X' || errno != EIO) {
        return 16;
    }
    single = strtof("0x1p2X", &end);
    if (single != 4.0f || *end != 'X' || errno != EIO) {
        return 17;
    }
    errno = EIO;
    single = strtof("1e999", &end);
    if (!is_float_infinity(single, 0) || *end != '\0' || errno != ERANGE) {
        return 18;
    }
    errno = EIO;
    single = strtof("1e-50", &end);
    if ((float_bits(single) & 0x7fffffffU) != 0U || *end != '\0' ||
        errno != ERANGE) {
        return 19;
    }

    errno = EIO;
    if (sscanf("INF 0x1.8p1 nan(tag)", "%lf %la %lf",
               &first, &second, &third) != 3 ||
        !is_double_infinity(first, 0) || second != 3.0 ||
        !is_double_nan(third) || errno != EIO) {
        return 20;
    }

    if (mini_sys_write(1, ok, sizeof(ok) - 1U) !=
        (long)(sizeof(ok) - 1U)) {
        return 21;
    }
    return 0;
}
