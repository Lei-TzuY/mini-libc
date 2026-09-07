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

static unsigned int float_bits(float value)
{
    union {
        float value;
        unsigned int bits;
    } convert;

    convert.value = value;
    return convert.bits;
}

static int double_is_nan_bits(unsigned long long bits)
{
    return (bits & MINI_DOUBLE_EXP) == MINI_DOUBLE_EXP &&
           (bits & MINI_DOUBLE_FRAC) != 0ULL;
}

static int float_is_nan_bits(unsigned int bits)
{
    return (bits & MINI_FLOAT_EXP) == MINI_FLOAT_EXP &&
           (bits & MINI_FLOAT_FRAC) != 0U;
}

int __mini_fpclassify(double x)
{
    unsigned long long bits = double_bits(x);
    unsigned long long exponent = bits & MINI_DOUBLE_EXP;
    unsigned long long fraction = bits & MINI_DOUBLE_FRAC;

    if (exponent == MINI_DOUBLE_EXP) {
        return fraction != 0ULL ? FP_NAN : FP_INFINITE;
    }
    if (exponent == 0ULL) {
        return fraction != 0ULL ? FP_SUBNORMAL : FP_ZERO;
    }
    return FP_NORMAL;
}

int __mini_fpclassifyf(float x)
{
    unsigned int bits = float_bits(x);
    unsigned int exponent = bits & MINI_FLOAT_EXP;
    unsigned int fraction = bits & MINI_FLOAT_FRAC;

    if (exponent == MINI_FLOAT_EXP) {
        return fraction != 0U ? FP_NAN : FP_INFINITE;
    }
    if (exponent == 0U) {
        return fraction != 0U ? FP_SUBNORMAL : FP_ZERO;
    }
    return FP_NORMAL;
}

int __mini_math_predicate(double x, int operation)
{
    int classification = __mini_fpclassify(x);

    if (operation == 0) {
        return classification != FP_INFINITE && classification != FP_NAN;
    }
    if (operation == 1) {
        return classification == FP_INFINITE;
    }
    if (operation == 2) {
        return classification == FP_NAN;
    }
    return classification == FP_NORMAL;
}

int __mini_math_predicatef(float x, int operation)
{
    int classification = __mini_fpclassifyf(x);

    if (operation == 0) {
        return classification != FP_INFINITE && classification != FP_NAN;
    }
    if (operation == 1) {
        return classification == FP_INFINITE;
    }
    if (operation == 2) {
        return classification == FP_NAN;
    }
    return classification == FP_NORMAL;
}

int __mini_signbit(double x)
{
    return (double_bits(x) & MINI_DOUBLE_SIGN) != 0ULL;
}

int __mini_signbitf(float x)
{
    return (float_bits(x) & MINI_FLOAT_SIGN) != 0U;
}

int __mini_math_compare(double x, double y, int operation)
{
    if (double_is_nan_bits(double_bits(x)) || double_is_nan_bits(double_bits(y))) {
        return operation == 0;
    }
    if (operation == 0) {
        return 0;
    }
    if (operation == 1) {
        return x > y;
    }
    if (operation == 2) {
        return x >= y;
    }
    if (operation == 3) {
        return x < y;
    }
    if (operation == 4) {
        return x <= y;
    }
    return x != y;
}

int __mini_math_comparef(float x, float y, int operation)
{
    if (float_is_nan_bits(float_bits(x)) || float_is_nan_bits(float_bits(y))) {
        return operation == 0;
    }
    if (operation == 0) {
        return 0;
    }
    if (operation == 1) {
        return x > y;
    }
    if (operation == 2) {
        return x >= y;
    }
    if (operation == 3) {
        return x < y;
    }
    if (operation == 4) {
        return x <= y;
    }
    return x != y;
}
