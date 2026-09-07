#include <errno.h>
#include <math.h>

#define MINI_DOUBLE_SIGN 0x8000000000000000ULL
#define MINI_DOUBLE_EXP  0x7ff0000000000000ULL
#define MINI_DOUBLE_FRAC 0x000fffffffffffffULL
#define MINI_FLOAT_EXP   0x7f800000U
#define MINI_FLOAT_FRAC  0x007fffffU

#define MINI_LN2_HI 6.93147180369123816490e-01
#define MINI_LN2_LO 1.90821492927058770002e-10
#define MINI_INV_LN2 1.44269504088896338700e+00
#define MINI_SQRT_HALF 7.07106781186547524401e-01
#define MINI_EXP_OVERFLOW 7.09782712893383973096e+02
#define MINI_EXP_UNDERFLOW_ZERO -7.45133219101941108420e+02

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

static double exp_reduced(double r)
{
    double p = 1.60590438368216145994e-10;

    p = p * r + 2.08767569878680989792e-09;
    p = p * r + 2.50521083854417187751e-08;
    p = p * r + 2.75573192239858906526e-07;
    p = p * r + 2.75573192239858925112e-06;
    p = p * r + 2.48015873015873015844e-05;
    p = p * r + 1.98412698412698412526e-04;
    p = p * r + 1.38888888888888894189e-03;
    p = p * r + 8.33333333333333321769e-03;
    p = p * r + 4.16666666666666643537e-02;
    p = p * r + 1.66666666666666657415e-01;
    p = p * r + 5.00000000000000000000e-01;
    p = p * r + 1.00000000000000000000e+00;
    p = p * r + 1.00000000000000000000e+00;
    return p;
}

double exp(double x)
{
    unsigned long long bits = double_bits(x);
    unsigned long long magnitude = bits & ~MINI_DOUBLE_SIGN;
    double scaled;
    double z;
    double r;
    int n;

    if (double_is_nan(bits)) {
        return x;
    }
    if (magnitude == MINI_DOUBLE_EXP) {
        if ((bits & MINI_DOUBLE_SIGN) != 0ULL) {
            return 0.0;
        }
        return x;
    }
    if (x > MINI_EXP_OVERFLOW) {
        errno = ERANGE;
        return double_from_bits(MINI_DOUBLE_EXP);
    }
    if (x < MINI_EXP_UNDERFLOW_ZERO) {
        errno = ERANGE;
        return 0.0;
    }

    z = x * MINI_INV_LN2;
    if (z >= 0.0) {
        n = (int)(z + 0.5);
    } else {
        n = (int)(z - 0.5);
    }
    r = (x - (double)n * MINI_LN2_HI) - (double)n * MINI_LN2_LO;
    scaled = scalbn(exp_reduced(r), n);
    bits = double_bits(scaled);

    if ((bits & MINI_DOUBLE_EXP) == MINI_DOUBLE_EXP) {
        errno = ERANGE;
    } else if ((bits & MINI_DOUBLE_EXP) == 0ULL) {
        errno = ERANGE;
    }
    return scaled;
}

float expf(float x)
{
    unsigned int bits = float_bits(x);
    unsigned int magnitude = bits & 0x7fffffffU;
    double wide;
    float result;
    unsigned int result_bits;

    if ((bits & MINI_FLOAT_EXP) == MINI_FLOAT_EXP &&
        (bits & MINI_FLOAT_FRAC) != 0U) {
        return x;
    }
    if (magnitude == MINI_FLOAT_EXP) {
        if ((bits & 0x80000000U) != 0U) {
            return 0.0f;
        }
        return x;
    }

    wide = exp((double)x);
    result = (float)wide;
    result_bits = float_bits(result);
    if ((result_bits & MINI_FLOAT_EXP) == MINI_FLOAT_EXP ||
        (result_bits & MINI_FLOAT_EXP) == 0U) {
        errno = ERANGE;
    }
    return result;
}

static double log_reduced(double m)
{
    double y = (m - 1.0) / (m + 1.0);
    double y2 = y * y;
    double term = y;
    double sum = term;
    int k;

    for (k = 1; k < 13; ++k) {
        term *= y2;
        sum += term / (double)(2 * k + 1);
    }
    return 2.0 * sum;
}

double log(double x)
{
    unsigned long long bits = double_bits(x);
    unsigned long long magnitude = bits & ~MINI_DOUBLE_SIGN;
    double m;
    double reduced;
    int exponent;

    if (double_is_nan(bits)) {
        return x;
    }
    if (magnitude == 0ULL) {
        errno = ERANGE;
        return double_from_bits(MINI_DOUBLE_SIGN | MINI_DOUBLE_EXP);
    }
    if ((bits & MINI_DOUBLE_SIGN) != 0ULL) {
        errno = EDOM;
        return double_from_bits(0x7ff8000000000000ULL);
    }
    if (magnitude == MINI_DOUBLE_EXP) {
        return x;
    }

    m = frexp(x, &exponent);
    if (m < MINI_SQRT_HALF) {
        m *= 2.0;
        --exponent;
    }
    reduced = log_reduced(m);
    return reduced + (double)exponent * MINI_LN2_HI +
           (double)exponent * MINI_LN2_LO;
}

float logf(float x)
{
    unsigned int bits = float_bits(x);

    if ((bits & MINI_FLOAT_EXP) == MINI_FLOAT_EXP &&
        (bits & MINI_FLOAT_FRAC) != 0U) {
        return x;
    }
    return (float)log((double)x);
}
