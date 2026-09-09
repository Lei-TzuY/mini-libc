#include <errno.h>
#include <fenv.h>
#include <math.h>

#define MINI_DOUBLE_SIGN 0x8000000000000000ULL
#define MINI_DOUBLE_EXP  0x7ff0000000000000ULL
#define MINI_DOUBLE_FRAC 0x000fffffffffffffULL
#define MINI_FLOAT_SIGN  0x80000000U
#define MINI_FLOAT_EXP   0x7f800000U
#define MINI_FLOAT_FRAC  0x007fffffU

#define MINI_LN2 6.93147180559945309417e-01
#define MINI_HYP_OVERFLOW 7.104758600739439771e+02
#define MINI_HYP_LARGE 2.0e+01
#define MINI_HYP_SQUARE_SAFE 1.0e+154

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

static int double_is_nan(unsigned long long bits)
{
    return (bits & MINI_DOUBLE_EXP) == MINI_DOUBLE_EXP &&
           (bits & MINI_DOUBLE_FRAC) != 0ULL;
}

static int float_is_nan(unsigned int bits)
{
    return (bits & MINI_FLOAT_EXP) == MINI_FLOAT_EXP &&
           (bits & MINI_FLOAT_FRAC) != 0U;
}

static double absolute_double(unsigned long long bits)
{
    return double_from_bits(bits & ~MINI_DOUBLE_SIGN);
}

static double signed_infinity(unsigned long long sign)
{
    return double_from_bits((sign & MINI_DOUBLE_SIGN) | MINI_DOUBLE_EXP);
}

static double quiet_nan(void)
{
    return double_from_bits(0x7ff8000000000000ULL);
}

static void raise_overflow(void)
{
    errno = ERANGE;
    (void)feraiseexcept(FE_OVERFLOW | FE_INEXACT);
}

static double domain_nan(void)
{
    errno = EDOM;
    (void)feraiseexcept(FE_INVALID);
    return quiet_nan();
}

static double pole_infinity(unsigned long long sign)
{
    errno = ERANGE;
    (void)feraiseexcept(FE_DIVBYZERO);
    return signed_infinity(sign);
}

static double sinh_small(double x)
{
    double x2 = x * x;
    double p = 1.0 / 6227020800.0;

    p = p * x2 + 1.0 / 39916800.0;
    p = p * x2 + 1.0 / 362880.0;
    p = p * x2 + 1.0 / 5040.0;
    p = p * x2 + 1.0 / 120.0;
    p = p * x2 + 1.0 / 6.0;
    return x + x * x2 * p;
}

static double cosh_small(double x)
{
    double x2 = x * x;
    double p = 1.0 / 479001600.0;

    p = p * x2 + 1.0 / 3628800.0;
    p = p * x2 + 1.0 / 40320.0;
    p = p * x2 + 1.0 / 720.0;
    p = p * x2 + 1.0 / 24.0;
    p = p * x2 + 1.0 / 2.0;
    return 1.0 + x2 * p;
}

static double asinh_small(double x)
{
    double x2 = x * x;
    double term = x;
    double sum = x;
    int n;

    for (n = 0; n < 11; ++n) {
        double odd = (double)(2 * n + 1);
        double denominator = (double)(2 * (n + 1) * (2 * n + 3));

        term *= -x2 * odd * odd / denominator;
        sum += term;
    }
    return sum;
}

static double atanh_small(double x)
{
    double x2 = x * x;
    double term = x;
    double sum = x;
    int n;

    for (n = 1; n < 12; ++n) {
        term *= x2;
        sum += term / (double)(2 * n + 1);
    }
    return sum;
}

double sinh(double x)
{
    unsigned long long bits = double_bits(x);
    unsigned long long magnitude_bits = bits & ~MINI_DOUBLE_SIGN;
    double magnitude;
    double result;

    if (double_is_nan(bits)) {
        return x;
    }
    if (magnitude_bits == MINI_DOUBLE_EXP || magnitude_bits == 0ULL) {
        return x;
    }

    magnitude = absolute_double(bits);
    if (magnitude <= 0.5) {
        return sinh_small(x);
    }
    if (magnitude > MINI_HYP_OVERFLOW) {
        raise_overflow();
        return signed_infinity(bits);
    }
    if (magnitude > MINI_HYP_LARGE) {
        result = exp(magnitude - MINI_LN2);
    } else {
        double positive = exp(magnitude);
        double negative = 1.0 / positive;

        result = 0.5 * (positive - negative);
    }
    return (bits & MINI_DOUBLE_SIGN) != 0ULL ? -result : result;
}

double cosh(double x)
{
    unsigned long long bits = double_bits(x);
    unsigned long long magnitude_bits = bits & ~MINI_DOUBLE_SIGN;
    double magnitude;

    if (double_is_nan(bits)) {
        return x;
    }
    if (magnitude_bits == MINI_DOUBLE_EXP) {
        return double_from_bits(MINI_DOUBLE_EXP);
    }

    magnitude = absolute_double(bits);
    if (magnitude <= 0.5) {
        return cosh_small(magnitude);
    }
    if (magnitude > MINI_HYP_OVERFLOW) {
        raise_overflow();
        return double_from_bits(MINI_DOUBLE_EXP);
    }
    if (magnitude > MINI_HYP_LARGE) {
        return exp(magnitude - MINI_LN2);
    }
    {
        double positive = exp(magnitude);
        double negative = 1.0 / positive;

        return 0.5 * (positive + negative);
    }
}

double tanh(double x)
{
    unsigned long long bits = double_bits(x);
    unsigned long long magnitude_bits = bits & ~MINI_DOUBLE_SIGN;
    double magnitude;
    double result;

    if (double_is_nan(bits)) {
        return x;
    }
    if (magnitude_bits == 0ULL) {
        return x;
    }
    if (magnitude_bits == MINI_DOUBLE_EXP) {
        return (bits & MINI_DOUBLE_SIGN) != 0ULL ? -1.0 : 1.0;
    }

    magnitude = absolute_double(bits);
    if (magnitude <= 0.5) {
        return sinh_small(x) / cosh_small(x);
    }
    if (magnitude > MINI_HYP_LARGE) {
        (void)feraiseexcept(FE_INEXACT);
        return (bits & MINI_DOUBLE_SIGN) != 0ULL ? -1.0 : 1.0;
    }
    {
        double positive = exp(magnitude);
        double negative = 1.0 / positive;

        result = (positive - negative) / (positive + negative);
    }
    return (bits & MINI_DOUBLE_SIGN) != 0ULL ? -result : result;
}

double asinh(double x)
{
    unsigned long long bits = double_bits(x);
    unsigned long long magnitude_bits = bits & ~MINI_DOUBLE_SIGN;
    double magnitude;
    double result;

    if (double_is_nan(bits) || magnitude_bits == MINI_DOUBLE_EXP ||
        magnitude_bits == 0ULL) {
        return x;
    }

    magnitude = absolute_double(bits);
    if (magnitude <= 0.25) {
        return asinh_small(x);
    }
    if (magnitude > MINI_HYP_SQUARE_SAFE) {
        result = log(magnitude) + MINI_LN2;
    } else {
        double square = magnitude * magnitude;
        double root = sqrt(1.0 + square);
        double inside = 1.0 + magnitude + square / (1.0 + root);

        result = log(inside);
    }
    return (bits & MINI_DOUBLE_SIGN) != 0ULL ? -result : result;
}

double acosh(double x)
{
    unsigned long long bits = double_bits(x);
    unsigned long long magnitude_bits = bits & ~MINI_DOUBLE_SIGN;

    if (double_is_nan(bits)) {
        return x;
    }
    if ((bits & MINI_DOUBLE_SIGN) != 0ULL || x < 1.0) {
        return domain_nan();
    }
    if (x == 1.0) {
        return 0.0;
    }
    if (magnitude_bits == MINI_DOUBLE_EXP) {
        return x;
    }
    if (x > MINI_HYP_SQUARE_SAFE) {
        return log(x) + MINI_LN2;
    }
    return log(x + sqrt((x - 1.0) * (x + 1.0)));
}

double atanh(double x)
{
    unsigned long long bits = double_bits(x);
    unsigned long long magnitude_bits = bits & ~MINI_DOUBLE_SIGN;
    double magnitude;
    double result;

    if (double_is_nan(bits) || magnitude_bits == 0ULL) {
        return x;
    }

    magnitude = absolute_double(bits);
    if (magnitude > 1.0) {
        return domain_nan();
    }
    if (magnitude == 1.0) {
        return pole_infinity(bits);
    }
    if (magnitude <= 0.25) {
        return atanh_small(x);
    }

    result = 0.5 * log((1.0 + magnitude) / (1.0 - magnitude));
    return (bits & MINI_DOUBLE_SIGN) != 0ULL ? -result : result;
}

float sinhf(float x)
{
    unsigned int bits = float_bits(x);
    float result;

    if (float_is_nan(bits)) {
        return x;
    }
    result = (float)sinh((double)x);
    if ((bits & MINI_FLOAT_EXP) != MINI_FLOAT_EXP &&
        (float_bits(result) & MINI_FLOAT_EXP) == MINI_FLOAT_EXP) {
        raise_overflow();
    }
    return result;
}

float coshf(float x)
{
    unsigned int bits = float_bits(x);
    float result;

    if (float_is_nan(bits)) {
        return x;
    }
    result = (float)cosh((double)x);
    if ((bits & MINI_FLOAT_EXP) != MINI_FLOAT_EXP &&
        (float_bits(result) & MINI_FLOAT_EXP) == MINI_FLOAT_EXP) {
        raise_overflow();
    }
    return result;
}

float tanhf(float x)
{
    unsigned int bits = float_bits(x);

    if (float_is_nan(bits)) {
        return x;
    }
    return (float)tanh((double)x);
}

float asinhf(float x)
{
    unsigned int bits = float_bits(x);

    if (float_is_nan(bits)) {
        return x;
    }
    return (float)asinh((double)x);
}

float acoshf(float x)
{
    unsigned int bits = float_bits(x);

    if (float_is_nan(bits)) {
        return x;
    }
    return (float)acosh((double)x);
}

float atanhf(float x)
{
    unsigned int bits = float_bits(x);

    if (float_is_nan(bits)) {
        return x;
    }
    return (float)atanh((double)x);
}
