#include <errno.h>
#include <math.h>

#define MINI_DOUBLE_SIGN 0x8000000000000000ULL
#define MINI_DOUBLE_EXP  0x7ff0000000000000ULL
#define MINI_DOUBLE_FRAC 0x000fffffffffffffULL
#define MINI_FLOAT_SIGN 0x80000000U
#define MINI_FLOAT_EXP  0x7f800000U
#define MINI_FLOAT_FRAC 0x007fffffU

static unsigned long long double_bits(double value)
{
    union {
        double value;
        unsigned long long bits;
    } convert;

    convert.value = value;
    return convert.bits;
}

static double double_from_bits(unsigned long long bits)
{
    union {
        double value;
        unsigned long long bits;
    } convert;

    convert.bits = bits;
    return convert.value;
}

static unsigned int float_bits(float value)
{
    union {
        float value;
        unsigned int bits;
    } convert;

    convert.value = value;
    return convert.bits;
}

static float float_from_bits(unsigned int bits)
{
    union {
        float value;
        unsigned int bits;
    } convert;

    convert.bits = bits;
    return convert.value;
}

static int highest_bit_u64(unsigned long long value)
{
    int bit = -1;

    while (value != 0ULL) {
        ++bit;
        value >>= 1;
    }
    return bit;
}

static double trunc_bits_double(double x)
{
    unsigned long long bits = double_bits(x);
    unsigned int exponent_field =
        (unsigned int)((bits & MINI_DOUBLE_EXP) >> 52);
    int exponent;
    unsigned long long mask;

    if (exponent_field == 0x7ffU) {
        return x;
    }
    exponent = (int)exponent_field - 1023;
    if (exponent < 0) {
        return double_from_bits(bits & MINI_DOUBLE_SIGN);
    }
    if (exponent >= 52) {
        return x;
    }
    mask = (1ULL << (52 - exponent)) - 1ULL;
    return double_from_bits(bits & ~mask);
}

static float trunc_bits_float(float x)
{
    unsigned int bits = float_bits(x);
    unsigned int exponent_field = (bits & MINI_FLOAT_EXP) >> 23;
    int exponent;
    unsigned int mask;

    if (exponent_field == 0xffU) {
        return x;
    }
    exponent = (int)exponent_field - 127;
    if (exponent < 0) {
        return float_from_bits(bits & MINI_FLOAT_SIGN);
    }
    if (exponent >= 23) {
        return x;
    }
    mask = (1U << (23 - exponent)) - 1U;
    return float_from_bits(bits & ~mask);
}

double frexp(double x, int *exp)
{
    unsigned long long bits = double_bits(x);
    unsigned long long sign = bits & MINI_DOUBLE_SIGN;
    unsigned int exponent_field =
        (unsigned int)((bits & MINI_DOUBLE_EXP) >> 52);
    unsigned long long fraction = bits & MINI_DOUBLE_FRAC;

    if (exponent_field == 0x7ffU ||
        (exponent_field == 0U && fraction == 0ULL)) {
        *exp = 0;
        return x;
    }

    if (exponent_field != 0U) {
        *exp = (int)exponent_field - 1022;
        return double_from_bits(sign | (1022ULL << 52) | fraction);
    }

    {
        int top = highest_bit_u64(fraction);
        unsigned long long significand = fraction << (52 - top);

        *exp = top - 1073;
        return double_from_bits(sign | (1022ULL << 52) |
                                (significand & MINI_DOUBLE_FRAC));
    }
}

float frexpf(float x, int *exp)
{
    unsigned int bits = float_bits(x);
    unsigned int sign = bits & MINI_FLOAT_SIGN;
    unsigned int exponent_field = (bits & MINI_FLOAT_EXP) >> 23;
    unsigned int fraction = bits & MINI_FLOAT_FRAC;

    if (exponent_field == 0xffU ||
        (exponent_field == 0U && fraction == 0U)) {
        *exp = 0;
        return x;
    }

    if (exponent_field != 0U) {
        *exp = (int)exponent_field - 126;
        return float_from_bits(sign | (126U << 23) | fraction);
    }

    {
        int top = highest_bit_u64(fraction);
        unsigned int significand = fraction << (23 - top);

        *exp = top - 148;
        return float_from_bits(sign | (126U << 23) |
                               (significand & MINI_FLOAT_FRAC));
    }
}

double modf(double x, double *iptr)
{
    unsigned long long bits = double_bits(x);
    unsigned int exponent_field =
        (unsigned int)((bits & MINI_DOUBLE_EXP) >> 52);
    unsigned long long fraction = bits & MINI_DOUBLE_FRAC;
    double integral;
    double fractional;

    if (exponent_field == 0x7ffU) {
        *iptr = x;
        if (fraction != 0ULL) {
            return x;
        }
        return double_from_bits(bits & MINI_DOUBLE_SIGN);
    }

    integral = trunc_bits_double(x);
    *iptr = integral;
    fractional = x - integral;
    if (fractional == 0.0) {
        return double_from_bits(bits & MINI_DOUBLE_SIGN);
    }
    return fractional;
}

float modff(float x, float *iptr)
{
    unsigned int bits = float_bits(x);
    unsigned int exponent_field = (bits & MINI_FLOAT_EXP) >> 23;
    unsigned int fraction = bits & MINI_FLOAT_FRAC;
    float integral;
    float fractional;

    if (exponent_field == 0xffU) {
        *iptr = x;
        if (fraction != 0U) {
            return x;
        }
        return float_from_bits(bits & MINI_FLOAT_SIGN);
    }

    integral = trunc_bits_float(x);
    *iptr = integral;
    fractional = x - integral;
    if (fractional == 0.0f) {
        return float_from_bits(bits & MINI_FLOAT_SIGN);
    }
    return fractional;
}
