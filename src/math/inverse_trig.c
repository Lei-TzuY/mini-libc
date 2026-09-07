#include <errno.h>
#include <math.h>

#define MINI_DOUBLE_SIGN 0x8000000000000000ULL
#define MINI_DOUBLE_EXP  0x7ff0000000000000ULL
#define MINI_DOUBLE_FRAC 0x000fffffffffffffULL
#define MINI_FLOAT_EXP   0x7f800000U
#define MINI_FLOAT_FRAC  0x007fffffU

#define MINI_PI          3.14159265358979323846264338327950288
#define MINI_PIO2        1.57079632679489661923132169163975144
#define MINI_PIO4        0.785398163397448309615660845819875721
#define MINI_TAN_PI8     0.414213562373095048801688724209698079
#define MINI_TAN_3PI8    2.414213562373095048801688724209698079

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

static double invalid_inverse_trig(void)
{
    errno = EDOM;
    return double_from_bits(0x7ff8000000000000ULL);
}

static double atan_kernel(double x)
{
    double z = x * x;
    double term = x;
    double sum = x;
    unsigned int n;

    for (n = 1U; n <= 24U; ++n) {
        term *= -z;
        sum += term / (double)(2U * n + 1U);
    }
    return sum;
}

static double atan_positive(double x)
{
    if (x > MINI_TAN_3PI8) {
        return MINI_PIO2 - atan_kernel(1.0 / x);
    }
    if (x > MINI_TAN_PI8) {
        return MINI_PIO4 + atan_kernel((x - 1.0) / (x + 1.0));
    }
    return atan_kernel(x);
}

double atan(double x)
{
    unsigned long long bits = double_bits(x);
    unsigned long long magnitude = bits & ~MINI_DOUBLE_SIGN;
    double result;

    if (double_is_nan(bits)) {
        return x;
    }
    if (magnitude == MINI_DOUBLE_EXP) {
        return (bits & MINI_DOUBLE_SIGN) != 0ULL ? -MINI_PIO2 : MINI_PIO2;
    }
    if (magnitude == 0ULL) {
        return x;
    }

    result = atan_positive(double_from_bits(magnitude));
    return (bits & MINI_DOUBLE_SIGN) != 0ULL ? -result : result;
}

double atan2(double y, double x)
{
    unsigned long long y_bits = double_bits(y);
    unsigned long long x_bits = double_bits(x);
    unsigned long long y_magnitude = y_bits & ~MINI_DOUBLE_SIGN;
    unsigned long long x_magnitude = x_bits & ~MINI_DOUBLE_SIGN;
    int y_negative = (y_bits & MINI_DOUBLE_SIGN) != 0ULL;
    int x_negative = (x_bits & MINI_DOUBLE_SIGN) != 0ULL;
    double ay;
    double ax;
    double base;

    if (double_is_nan(y_bits)) {
        return y;
    }
    if (double_is_nan(x_bits)) {
        return x;
    }

    if (y_magnitude == 0ULL) {
        if (x_negative) {
            return y_negative ? -MINI_PI : MINI_PI;
        }
        return y;
    }
    if (x_magnitude == 0ULL) {
        return y_negative ? -MINI_PIO2 : MINI_PIO2;
    }

    if (y_magnitude == MINI_DOUBLE_EXP) {
        if (x_magnitude == MINI_DOUBLE_EXP) {
            base = x_negative ? 3.0 * MINI_PIO4 : MINI_PIO4;
            return y_negative ? -base : base;
        }
        return y_negative ? -MINI_PIO2 : MINI_PIO2;
    }
    if (x_magnitude == MINI_DOUBLE_EXP) {
        if (x_negative) {
            return y_negative ? -MINI_PI : MINI_PI;
        }
        return double_from_bits(y_bits & MINI_DOUBLE_SIGN);
    }

    ay = double_from_bits(y_magnitude);
    ax = double_from_bits(x_magnitude);
    if (ay <= ax) {
        base = atan_positive(ay / ax);
    } else {
        base = MINI_PIO2 - atan_positive(ax / ay);
    }
    if (x_negative) {
        base = MINI_PI - base;
    }
    return y_negative ? -base : base;
}

double asin(double x)
{
    unsigned long long bits = double_bits(x);
    unsigned long long magnitude = bits & ~MINI_DOUBLE_SIGN;
    double ax;
    double root;
    double result;

    if (double_is_nan(bits)) {
        return x;
    }
    if (magnitude == 0ULL) {
        return x;
    }

    ax = double_from_bits(magnitude);
    if (ax > 1.0) {
        return invalid_inverse_trig();
    }
    if (ax == 1.0) {
        return (bits & MINI_DOUBLE_SIGN) != 0ULL ? -MINI_PIO2 : MINI_PIO2;
    }

    root = sqrt((1.0 - ax) * (1.0 + ax));
    result = atan2(ax, root);
    return (bits & MINI_DOUBLE_SIGN) != 0ULL ? -result : result;
}

double acos(double x)
{
    unsigned long long bits = double_bits(x);
    double root;

    if (double_is_nan(bits)) {
        return x;
    }
    if (x > 1.0 || x < -1.0) {
        return invalid_inverse_trig();
    }
    if (x == 1.0) {
        return 0.0;
    }
    if (x == -1.0) {
        return MINI_PI;
    }

    root = sqrt((1.0 - x) * (1.0 + x));
    return atan2(root, x);
}

float atanf(float x)
{
    if (float_is_nan(float_bits(x))) {
        return x;
    }
    return (float)atan((double)x);
}

float atan2f(float y, float x)
{
    if (float_is_nan(float_bits(y))) {
        return y;
    }
    if (float_is_nan(float_bits(x))) {
        return x;
    }
    return (float)atan2((double)y, (double)x);
}

float asinf(float x)
{
    if (float_is_nan(float_bits(x))) {
        return x;
    }
    return (float)asin((double)x);
}

float acosf(float x)
{
    if (float_is_nan(float_bits(x))) {
        return x;
    }
    return (float)acos((double)x);
}
