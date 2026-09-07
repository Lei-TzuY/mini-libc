#include <errno.h>
#include <math.h>

#define MINI_DOUBLE_SIGN 0x8000000000000000ULL
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

static float float_from_bits(unsigned int bits)
{
    union {
        float value;
        unsigned int bits;
    } convert;

    convert.bits = bits;
    return convert.value;
}

static double absolute_double(double value)
{
    return double_from_bits(double_bits(value) & ~MINI_DOUBLE_SIGN);
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
    double ax = absolute_double(x);
    double ay = absolute_double(y);
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
    double ax = absolute_double(x);
    double ay = absolute_double(y);
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
    int saved_errno;
    double result;

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
    saved_errno = errno;
    result = fmod_finite(x, y);
    errno = saved_errno;
    return result;
}

float fmodf(float x, float y)
{
    int saved_errno;
    float result;

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
    saved_errno = errno;
    result = (float)fmod_finite((double)x, (double)y);
    errno = saved_errno;
    return result;
}

double remainder(double x, double y)
{
    int saved_errno;
    double result;

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
    saved_errno = errno;
    result = remainder_finite(x, y, 0);
    errno = saved_errno;
    return result;
}

float remainderf(float x, float y)
{
    int saved_errno;
    float result;

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
    saved_errno = errno;
    result = (float)remainder_finite((double)x, (double)y, 0);
    errno = saved_errno;
    return result;
}

double remquo(double x, double y, int *quo)
{
    int saved_errno;
    double result;

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
    saved_errno = errno;
    result = remainder_finite(x, y, quo);
    errno = saved_errno;
    return result;
}

float remquof(float x, float y, int *quo)
{
    int saved_errno;
    float result;

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
    saved_errno = errno;
    result = (float)remainder_finite((double)x, (double)y, quo);
    errno = saved_errno;
    return result;
}
