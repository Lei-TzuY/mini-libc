#include <errno.h>
#include <math.h>

#define MINI_DOUBLE_SIGN 0x8000000000000000ULL
#define MINI_DOUBLE_EXP  0x7ff0000000000000ULL
#define MINI_DOUBLE_FRAC 0x000fffffffffffffULL
#define MINI_FLOAT_SIGN 0x80000000U
#define MINI_FLOAT_EXP  0x7f800000U
#define MINI_FLOAT_FRAC 0x007fffffU
#define MINI_REMQUO_MASK 0x7fU

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

struct mini_reduction {
    double remainder_unit;
    double divisor_unit;
    int divisor_exponent;
    unsigned int quotient_low;
};

static struct mini_reduction reduce_positive(double x, double y)
{
    struct mini_reduction result;
    double remainder;
    double divisor;
    int x_exponent;
    int y_exponent;
    int shifts;
    int index;
    unsigned int quotient = 0U;

    remainder = frexp(x, &x_exponent);
    divisor = frexp(y, &y_exponent);
    shifts = x_exponent - y_exponent;

    for (index = 0; index <= shifts; ++index) {
        unsigned int bit = 0U;

        if (remainder >= divisor) {
            remainder -= divisor;
            bit = 1U;
        }
        quotient = ((quotient << 1) | bit) & MINI_REMQUO_MASK;
        if (index != shifts) {
            remainder *= 2.0;
        }
    }

    result.remainder_unit = remainder;
    result.divisor_unit = divisor;
    result.divisor_exponent = y_exponent;
    result.quotient_low = quotient;
    return result;
}

static double quiet_nan(void)
{
    return double_from_bits(0x7ff8000000000000ULL);
}

static float quiet_nanf(void)
{
    return float_from_bits(0x7fc00000U);
}

static int remainder_domain(double x, double y)
{
    return isinf(x) || y == 0.0;
}

static double signed_zero_like(double x)
{
    return double_from_bits(double_bits(x) & MINI_DOUBLE_SIGN);
}

static double fmod_finite(double x, double y)
{
    double ax = fabs(x);
    double ay = fabs(y);
    struct mini_reduction reduction;
    double result;

    if (ax < ay) {
        return x;
    }
    if (ax == ay) {
        return signed_zero_like(x);
    }

    reduction = reduce_positive(ax, ay);
    result = scalbn(reduction.remainder_unit, reduction.divisor_exponent);
    if (signbit(x)) {
        result = -result;
    }
    if (result == 0.0) {
        return signed_zero_like(x);
    }
    return result;
}

static double remainder_finite(double x, double y, int *quotient_out)
{
    double ax = fabs(x);
    double ay = fabs(y);
    double result;
    unsigned int quotient;
    int quotient_negative = signbit(x) != signbit(y);

    if (ax < ay) {
        double opposite = ay - ax;

        if (ax > opposite) {
            result = ax - ay;
            quotient = 1U;
        } else {
            result = ax;
            quotient = 0U;
        }
    } else if (ax == ay) {
        result = 0.0;
        quotient = 1U;
    } else {
        struct mini_reduction reduction = reduce_positive(ax, ay);
        double twice = reduction.remainder_unit * 2.0;

        quotient = reduction.quotient_low;
        if (twice > reduction.divisor_unit ||
            (twice == reduction.divisor_unit && (quotient & 1U) != 0U)) {
            reduction.remainder_unit -= reduction.divisor_unit;
            quotient = (quotient + 1U) & MINI_REMQUO_MASK;
        }
        result = scalbn(reduction.remainder_unit, reduction.divisor_exponent);
    }

    if (signbit(x)) {
        result = -result;
    }
    if (result == 0.0) {
        result = signed_zero_like(x);
    }
    if (quotient_out != 0) {
        int q = (int)(quotient & MINI_REMQUO_MASK);

        *quotient_out = quotient_negative ? -q : q;
    }
    return result;
}

double fmod(double x, double y)
{
    if (isnan(x)) {
        return x;
    }
    if (isnan(y)) {
        return y;
    }
    if (remainder_domain(x, y)) {
        errno = EDOM;
        return quiet_nan();
    }
    if (isinf(y) || x == 0.0) {
        return x;
    }
    return fmod_finite(x, y);
}

float fmodf(float x, float y)
{
    if (isnan(x)) {
        return x;
    }
    if (isnan(y)) {
        return y;
    }
    if (isinf(x) || y == 0.0f) {
        errno = EDOM;
        return quiet_nanf();
    }
    if (isinf(y) || x == 0.0f) {
        return x;
    }
    return (float)fmod_finite((double)x, (double)y);
}

double remainder(double x, double y)
{
    if (isnan(x)) {
        return x;
    }
    if (isnan(y)) {
        return y;
    }
    if (remainder_domain(x, y)) {
        errno = EDOM;
        return quiet_nan();
    }
    if (isinf(y) || x == 0.0) {
        return x;
    }
    return remainder_finite(x, y, 0);
}

float remainderf(float x, float y)
{
    if (isnan(x)) {
        return x;
    }
    if (isnan(y)) {
        return y;
    }
    if (isinf(x) || y == 0.0f) {
        errno = EDOM;
        return quiet_nanf();
    }
    if (isinf(y) || x == 0.0f) {
        return x;
    }
    return (float)remainder_finite((double)x, (double)y, 0);
}

double remquo(double x, double y, int *quo)
{
    if (quo != 0) {
        *quo = 0;
    }
    if (isnan(x)) {
        return x;
    }
    if (isnan(y)) {
        return y;
    }
    if (remainder_domain(x, y)) {
        errno = EDOM;
        return quiet_nan();
    }
    if (isinf(y) || x == 0.0) {
        return x;
    }
    return remainder_finite(x, y, quo);
}

float remquof(float x, float y, int *quo)
{
    if (quo != 0) {
        *quo = 0;
    }
    if (isnan(x)) {
        return x;
    }
    if (isnan(y)) {
        return y;
    }
    if (isinf(x) || y == 0.0f) {
        errno = EDOM;
        return quiet_nanf();
    }
    if (isinf(y) || x == 0.0f) {
        return x;
    }
    return (float)remainder_finite((double)x, (double)y, quo);
}
