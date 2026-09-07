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

static int double_is_nan_bits(unsigned long long bits)
{
    return (bits & MINI_DOUBLE_EXP) == MINI_DOUBLE_EXP &&
           (bits & MINI_DOUBLE_FRAC) != 0ULL;
}

static int double_is_finite_bits(unsigned long long bits)
{
    return (bits & MINI_DOUBLE_EXP) != MINI_DOUBLE_EXP;
}

static int float_is_nan_bits(unsigned int bits)
{
    return (bits & MINI_FLOAT_EXP) == MINI_FLOAT_EXP &&
           (bits & MINI_FLOAT_FRAC) != 0U;
}

static int float_is_finite_bits(unsigned int bits)
{
    return (bits & MINI_FLOAT_EXP) != MINI_FLOAT_EXP;
}

double fabs(double x)
{
    return double_from_bits(double_bits(x) & ~MINI_DOUBLE_SIGN);
}

float fabsf(float x)
{
    return float_from_bits(float_bits(x) & ~MINI_FLOAT_SIGN);
}

double copysign(double x, double y)
{
    unsigned long long x_bits = double_bits(x) & ~MINI_DOUBLE_SIGN;
    unsigned long long y_bits = double_bits(y) & MINI_DOUBLE_SIGN;

    return double_from_bits(x_bits | y_bits);
}

float copysignf(float x, float y)
{
    unsigned int x_bits = float_bits(x) & ~MINI_FLOAT_SIGN;
    unsigned int y_bits = float_bits(y) & MINI_FLOAT_SIGN;

    return float_from_bits(x_bits | y_bits);
}

double fmin(double x, double y)
{
    unsigned long long x_bits = double_bits(x);
    unsigned long long y_bits = double_bits(y);

    if (double_is_nan_bits(x_bits)) {
        return y;
    }
    if (double_is_nan_bits(y_bits)) {
        return x;
    }
    if (x == y && x == 0.0) {
        return (x_bits & MINI_DOUBLE_SIGN) != 0ULL ? x : y;
    }
    return x < y ? x : y;
}

float fminf(float x, float y)
{
    unsigned int x_bits = float_bits(x);
    unsigned int y_bits = float_bits(y);

    if (float_is_nan_bits(x_bits)) {
        return y;
    }
    if (float_is_nan_bits(y_bits)) {
        return x;
    }
    if (x == y && x == 0.0f) {
        return (x_bits & MINI_FLOAT_SIGN) != 0U ? x : y;
    }
    return x < y ? x : y;
}

double fmax(double x, double y)
{
    unsigned long long x_bits = double_bits(x);
    unsigned long long y_bits = double_bits(y);

    if (double_is_nan_bits(x_bits)) {
        return y;
    }
    if (double_is_nan_bits(y_bits)) {
        return x;
    }
    if (x == y && x == 0.0) {
        return (x_bits & MINI_DOUBLE_SIGN) != 0ULL ? y : x;
    }
    return x > y ? x : y;
}

float fmaxf(float x, float y)
{
    unsigned int x_bits = float_bits(x);
    unsigned int y_bits = float_bits(y);

    if (float_is_nan_bits(x_bits)) {
        return y;
    }
    if (float_is_nan_bits(y_bits)) {
        return x;
    }
    if (x == y && x == 0.0f) {
        return (x_bits & MINI_FLOAT_SIGN) != 0U ? y : x;
    }
    return x > y ? x : y;
}

double trunc(double x)
{
    unsigned long long bits = double_bits(x);
    unsigned int exponent_field = (unsigned int)((bits & MINI_DOUBLE_EXP) >> 52);
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

float truncf(float x)
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

double floor(double x)
{
    unsigned long long bits = double_bits(x);
    double integral;

    if (!double_is_finite_bits(bits)) {
        return x;
    }
    integral = trunc(x);
    if (x < integral) {
        return integral - 1.0;
    }
    return integral;
}

float floorf(float x)
{
    unsigned int bits = float_bits(x);
    float integral;

    if (!float_is_finite_bits(bits)) {
        return x;
    }
    integral = truncf(x);
    if (x < integral) {
        return integral - 1.0f;
    }
    return integral;
}

double ceil(double x)
{
    unsigned long long bits = double_bits(x);
    double integral;

    if (!double_is_finite_bits(bits)) {
        return x;
    }
    integral = trunc(x);
    if (x > integral) {
        return integral + 1.0;
    }
    return integral;
}

float ceilf(float x)
{
    unsigned int bits = float_bits(x);
    float integral;

    if (!float_is_finite_bits(bits)) {
        return x;
    }
    integral = truncf(x);
    if (x > integral) {
        return integral + 1.0f;
    }
    return integral;
}

double round(double x)
{
    unsigned long long bits = double_bits(x);
    double integral;
    double fraction;

    if (!double_is_finite_bits(bits)) {
        return x;
    }
    integral = trunc(x);
    fraction = x - integral;
    if (fraction >= 0.5) {
        return integral + 1.0;
    }
    if (fraction <= -0.5) {
        return integral - 1.0;
    }
    return integral;
}

float roundf(float x)
{
    unsigned int bits = float_bits(x);
    float integral;
    float fraction;

    if (!float_is_finite_bits(bits)) {
        return x;
    }
    integral = truncf(x);
    fraction = x - integral;
    if (fraction >= 0.5f) {
        return integral + 1.0f;
    }
    if (fraction <= -0.5f) {
        return integral - 1.0f;
    }
    return integral;
}

extern double __mini_sqrt_hw(double x);
extern float __mini_sqrtf_hw(float x);

double sqrt(double x)
{
    unsigned long long bits = double_bits(x);
    unsigned long long magnitude = bits & ~MINI_DOUBLE_SIGN;

    if (double_is_nan_bits(bits)) {
        return x;
    }
    if ((bits & MINI_DOUBLE_SIGN) != 0ULL && magnitude != 0ULL) {
        errno = EDOM;
        return double_from_bits(0x7ff8000000000000ULL);
    }
    return __mini_sqrt_hw(x);
}

float sqrtf(float x)
{
    unsigned int bits = float_bits(x);
    unsigned int magnitude = bits & ~MINI_FLOAT_SIGN;

    if (float_is_nan_bits(bits)) {
        return x;
    }
    if ((bits & MINI_FLOAT_SIGN) != 0U && magnitude != 0U) {
        errno = EDOM;
        return float_from_bits(0x7fc00000U);
    }
    return __mini_sqrtf_hw(x);
}
