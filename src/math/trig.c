#include <errno.h>
#include <math.h>

#define MINI_DOUBLE_SIGN 0x8000000000000000ULL
#define MINI_DOUBLE_EXP  0x7ff0000000000000ULL
#define MINI_DOUBLE_FRAC 0x000fffffffffffffULL
#define MINI_FLOAT_EXP   0x7f800000U
#define MINI_FLOAT_FRAC  0x007fffffU

#define MINI_TRIG_BOUND 1048576.0
#define MINI_INV_PIO2 6.36619772367581382433e-01
#define MINI_PIO2_HI  1.57079632673412561417e+00
#define MINI_PIO2_LO  6.07710050650619224932e-11

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

static double invalid_trig(void)
{
    errno = EDOM;
    return double_from_bits(0x7ff8000000000000ULL);
}

static int reduce_pio2(double x, int *quadrant, double *reduced)
{
    double z;
    long long n;
    int q;

    if (x > MINI_TRIG_BOUND || x < -MINI_TRIG_BOUND) {
        return 0;
    }

    z = x * MINI_INV_PIO2;
    if (z >= 0.0) {
        n = (long long)(z + 0.5);
    } else {
        n = (long long)(z - 0.5);
    }

    *reduced = (x - (double)n * MINI_PIO2_HI) -
               (double)n * MINI_PIO2_LO;
    q = (int)(n % 4LL);
    if (q < 0) {
        q += 4;
    }
    *quadrant = q;
    return 1;
}

static double sin_kernel(double x)
{
    double z = x * x;
    double p = 2.81145725434552076320e-15;

    p = -7.64716373181981647590e-13 + z * p;
    p = 1.60590438368216133484e-10 + z * p;
    p = -2.50521083854417187751e-08 + z * p;
    p = 2.75573192239858925112e-06 + z * p;
    p = -1.98412698412698412526e-04 + z * p;
    p = 8.33333333333333321769e-03 + z * p;
    p = -1.66666666666666657415e-01 + z * p;
    return x + x * z * p;
}

static double cos_kernel(double x)
{
    double z = x * x;
    double p = 4.77947733238738529744e-14;

    p = -1.14707455977297247139e-11 + z * p;
    p = 2.08767569878680989792e-09 + z * p;
    p = -2.75573192239858906526e-07 + z * p;
    p = 2.48015873015873015844e-05 + z * p;
    p = -1.38888888888888894189e-03 + z * p;
    p = 4.16666666666666643537e-02 + z * p;
    p = -5.00000000000000000000e-01 + z * p;
    return 1.0 + z * p;
}

static void reduced_sincos(double x, double *sine, double *cosine)
{
    int quadrant;
    double r;
    double s;
    double c;

    if (!reduce_pio2(x, &quadrant, &r)) {
        *sine = invalid_trig();
        *cosine = *sine;
        return;
    }

    s = sin_kernel(r);
    c = cos_kernel(r);
    if (quadrant == 0) {
        *sine = s;
        *cosine = c;
    } else if (quadrant == 1) {
        *sine = c;
        *cosine = -s;
    } else if (quadrant == 2) {
        *sine = -s;
        *cosine = -c;
    } else {
        *sine = -c;
        *cosine = s;
    }
}

double sin(double x)
{
    unsigned long long bits = double_bits(x);
    unsigned long long magnitude = bits & ~MINI_DOUBLE_SIGN;
    double s;
    double c;

    if (double_is_nan(bits)) {
        return x;
    }
    if (magnitude == MINI_DOUBLE_EXP) {
        return invalid_trig();
    }
    if (magnitude == 0ULL) {
        return x;
    }

    reduced_sincos(x, &s, &c);
    return s;
}

double cos(double x)
{
    unsigned long long bits = double_bits(x);
    unsigned long long magnitude = bits & ~MINI_DOUBLE_SIGN;
    double s;
    double c;

    if (double_is_nan(bits)) {
        return x;
    }
    if (magnitude == MINI_DOUBLE_EXP) {
        return invalid_trig();
    }
    if (magnitude == 0ULL) {
        return 1.0;
    }

    reduced_sincos(x, &s, &c);
    return c;
}

double tan(double x)
{
    unsigned long long bits = double_bits(x);
    unsigned long long magnitude = bits & ~MINI_DOUBLE_SIGN;
    double s;
    double c;

    if (double_is_nan(bits)) {
        return x;
    }
    if (magnitude == MINI_DOUBLE_EXP) {
        return invalid_trig();
    }
    if (magnitude == 0ULL) {
        return x;
    }

    reduced_sincos(x, &s, &c);
    return s / c;
}

float sinf(float x)
{
    if (float_is_nan(float_bits(x))) {
        return x;
    }
    return (float)sin((double)x);
}

float cosf(float x)
{
    if (float_is_nan(float_bits(x))) {
        return x;
    }
    return (float)cos((double)x);
}

float tanf(float x)
{
    if (float_is_nan(float_bits(x))) {
        return x;
    }
    return (float)tan((double)x);
}
