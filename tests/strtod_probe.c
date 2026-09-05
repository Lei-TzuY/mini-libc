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

static double double_from_bits(unsigned long long bits)
{
    double value;
    unsigned char *out = (unsigned char *)&value;
    const unsigned char *in = (const unsigned char *)&bits;
    unsigned int i;

    for (i = 0U; i < 8U; ++i) {
        out[i] = in[i];
    }
    return value;
}

static int same_string(const char *left, const char *right)
{
    unsigned int i = 0U;

    while (left[i] != '\0' && right[i] != '\0') {
        if (left[i] != right[i]) {
            return 0;
        }
        ++i;
    }
    return left[i] == right[i];
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
    char format_buffer[256];
    char *end;
    double value;
    double first;
    double second;
    double third;
    float single;
    int formatted;

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

    errno = EIO;
    formatted = snprintf(format_buffer, sizeof(format_buffer),
                         "e=[%e][%.2E][%+012.2e]",
                         1.5, -125.0, 1.5);
    if (formatted !=
            (int)(sizeof("e=[1.500000e+00][-1.25E+02][+0001.50e+00]") - 1U) ||
        !same_string(format_buffer,
                     "e=[1.500000e+00][-1.25E+02][+0001.50e+00]") ||
        errno != EIO) {
        return 21;
    }

    formatted = snprintf(format_buffer, sizeof(format_buffer),
                         "g=[%g][%g][%#.5g][%G]",
                         123.0, 0.0000123, 12.0, 10000000.0);
    if (formatted !=
            (int)(sizeof("g=[123][1.23e-05][12.000][1E+07]") - 1U) ||
        !same_string(format_buffer,
                     "g=[123][1.23e-05][12.000][1E+07]") ||
        errno != EIO) {
        return 22;
    }

    formatted = snprintf(format_buffer, sizeof(format_buffer),
                         "a=[%a][%.3A][%a][%.0a][%#12.0a]",
                         1.5, 1.5, double_from_bits(1ULL), 1.5, 1.0);
    if (formatted !=
            (int)(sizeof("a=[0x1.8p+0][0X1.800P+0][0x0.0000000000001p-1022][0x2p+0][     0x1.p+0]") - 1U) ||
        !same_string(format_buffer,
                     "a=[0x1.8p+0][0X1.800P+0][0x0.0000000000001p-1022][0x2p+0][     0x1.p+0]") ||
        errno != EIO) {
        return 23;
    }

    formatted = snprintf(format_buffer, sizeof(format_buffer),
                         "s=[%F][%E][%G][%A]",
                         double_from_bits(0x7ff0000000000000ULL),
                         double_from_bits(0x7ff8000000000001ULL),
                         double_from_bits(0xfff0000000000000ULL),
                         double_from_bits(0x7ff8000000000001ULL));
    if (formatted != (int)(sizeof("s=[INF][NAN][-INF][NAN]") - 1U) ||
        !same_string(format_buffer, "s=[INF][NAN][-INF][NAN]") ||
        errno != EIO) {
        return 24;
    }

    if (mini_sys_write(1, ok, sizeof(ok) - 1U) !=
        (long)(sizeof(ok) - 1U)) {
        return 25;
    }
    return 0;
}
