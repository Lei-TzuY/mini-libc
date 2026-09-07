#include <errno.h>
#include <math.h>

#define MINI_DOUBLE_SIGN 0x8000000000000000ULL
#define MINI_DOUBLE_EXP  0x7ff0000000000000ULL
#define MINI_DOUBLE_FRAC 0x000fffffffffffffULL
#define MINI_FLOAT_EXP   0x7f800000U
#define MINI_FLOAT_FRAC  0x007fffffU

#define MINI_PI 3.141592653589793238462643383279502884
#define MINI_LOG_PI 1.14472988584940017414342735135305871165
#define MINI_LOG_SQRT_2PI 0.91893853320467274178032973640561763986
#define MINI_TWO_POW_52 4503599627370496.0

static const double lanczos_coefficients[] = {
    0.99999999999980993227684700473478,
    676.520368121885098567009190444019,
    -1259.13921672240287047156078755283,
    771.3234287776530788486528258894,
    -176.61502916214059906584551354,
    12.507343278686904814458936853,
    -0.13857109526572011689554707,
    9.984369578019570859563e-6,
    1.50563273514931155834e-7
};

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

static double quiet_nan(void)
{
    return double_from_bits(0x7ff8000000000000ULL);
}

static double signed_infinity(int negative)
{
    return double_from_bits((negative ? MINI_DOUBLE_SIGN : 0ULL) | MINI_DOUBLE_EXP);
}

static int double_is_nan_bits(unsigned long long bits)
{
    return (bits & MINI_DOUBLE_EXP) == MINI_DOUBLE_EXP &&
           (bits & MINI_DOUBLE_FRAC) != 0ULL;
}

static int signed_integer_is_odd(long long value)
{
    unsigned long long magnitude;

    if (value < 0) {
        magnitude = (unsigned long long)(-value);
    } else {
        magnitude = (unsigned long long)value;
    }
    return (magnitude & 1ULL) != 0ULL;
}

/*
 * Reduce x to sin(pi*x) without ever sending a large argument to the bounded
 * trigonometric runtime. Every representable binary64 value with magnitude at
 * least 2^52 is integral, so only smaller values need a fractional reduction.
 */
static double sin_pi(double x, int *pole)
{
    long long integer;
    double fraction;
    double reduced;
    double result;
    int saved_errno = errno;

    if (x >= MINI_TWO_POW_52 || x <= -MINI_TWO_POW_52) {
        *pole = 1;
        return 0.0;
    }

    integer = (long long)x;
    fraction = x - (double)integer;
    if (fraction < 0.0) {
        --integer;
        fraction += 1.0;
    }
    if (fraction == 0.0) {
        *pole = 1;
        return 0.0;
    }

    reduced = fraction > 0.5 ? 1.0 - fraction : fraction;
    result = sin(MINI_PI * reduced);
    if (signed_integer_is_odd(integer)) {
        result = -result;
    }

    errno = saved_errno;
    *pole = 0;
    return result;
}

static double lanczos_log_gamma_positive(double x)
{
    double z = x - 1.0;
    double sum = lanczos_coefficients[0];
    double t;
    double result;
    unsigned int i;
    int saved_errno = errno;

    for (i = 1U; i < 9U; ++i) {
        sum += lanczos_coefficients[i] / (z + (double)i);
    }

    t = z + 7.5;
    result = MINI_LOG_SQRT_2PI +
             (z + 0.5) * log(t) - t + log(sum);
    errno = saved_errno;
    return result;
}

/*
 * Compute log(|Gamma(x)|) and the sign of Gamma(x) for finite nonzero x.
 * Returns nonzero when x is a negative integer pole.
 */
static int gamma_logabs(double x, double *logabs, int *sign)
{
    if (x >= 0.5) {
        *logabs = lanczos_log_gamma_positive(x);
        *sign = 1;
        return 0;
    }

    {
        int pole = 0;
        double sine = sin_pi(x, &pole);
        double reflected;
        double sine_magnitude;
        int saved_errno = errno;

        if (pole) {
            return 1;
        }

        reflected = lanczos_log_gamma_positive(1.0 - x);
        sine_magnitude = sine < 0.0 ? -sine : sine;
        *logabs = MINI_LOG_PI - log(sine_magnitude) - reflected;
        *sign = sine < 0.0 ? -1 : 1;
        errno = saved_errno;
        return 0;
    }
}

double lgamma(double x)
{
    unsigned long long bits = double_bits(x);
    unsigned long long magnitude = bits & ~MINI_DOUBLE_SIGN;
    double result;
    int sign;
    int saved_errno = errno;

    if (double_is_nan_bits(bits)) {
        return x;
    }
    if (magnitude == MINI_DOUBLE_EXP) {
        return signed_infinity(0);
    }
    if (magnitude == 0ULL) {
        errno = ERANGE;
        return signed_infinity(0);
    }
    if (gamma_logabs(x, &result, &sign) != 0) {
        errno = ERANGE;
        return signed_infinity(0);
    }
    if ((double_bits(result) & MINI_DOUBLE_EXP) == MINI_DOUBLE_EXP) {
        errno = ERANGE;
        return signed_infinity(0);
    }

    errno = saved_errno;
    return result;
}

double tgamma(double x)
{
    unsigned long long bits = double_bits(x);
    unsigned long long magnitude = bits & ~MINI_DOUBLE_SIGN;
    double logabs;
    double result;
    int sign;
    int saved_errno = errno;

    if (double_is_nan_bits(bits)) {
        return x;
    }
    if (magnitude == MINI_DOUBLE_EXP) {
        if ((bits & MINI_DOUBLE_SIGN) != 0ULL) {
            errno = EDOM;
            return quiet_nan();
        }
        return x;
    }
    if (magnitude == 0ULL) {
        errno = ERANGE;
        return signed_infinity((bits & MINI_DOUBLE_SIGN) != 0ULL);
    }
    if (gamma_logabs(x, &logabs, &sign) != 0) {
        errno = EDOM;
        return quiet_nan();
    }

    errno = saved_errno;
    result = exp(logabs);
    bits = double_bits(result);
    if ((bits & MINI_DOUBLE_EXP) == MINI_DOUBLE_EXP ||
        (bits & MINI_DOUBLE_EXP) == 0ULL) {
        errno = ERANGE;
    } else {
        errno = saved_errno;
    }

    return sign < 0 ? -result : result;
}

float lgammaf(float x)
{
    unsigned int bits = float_bits(x);
    double wide;
    float result;
    unsigned int result_bits;
    int saved_errno = errno;
    int core_errno;

    if ((bits & MINI_FLOAT_EXP) == MINI_FLOAT_EXP &&
        (bits & MINI_FLOAT_FRAC) != 0U) {
        return x;
    }

    errno = saved_errno;
    wide = lgamma((double)x);
    core_errno = errno;
    result = (float)wide;

    if (core_errno != saved_errno) {
        return result;
    }

    result_bits = float_bits(result);
    if ((double_bits(wide) & MINI_DOUBLE_EXP) != MINI_DOUBLE_EXP &&
        wide != 0.0 &&
        ((result_bits & MINI_FLOAT_EXP) == 0U ||
         (result_bits & MINI_FLOAT_EXP) == MINI_FLOAT_EXP)) {
        errno = ERANGE;
    } else {
        errno = saved_errno;
    }
    return result;
}

float tgammaf(float x)
{
    unsigned int bits = float_bits(x);
    double wide;
    float result;
    unsigned int result_bits;
    int saved_errno = errno;
    int core_errno;

    if ((bits & MINI_FLOAT_EXP) == MINI_FLOAT_EXP &&
        (bits & MINI_FLOAT_FRAC) != 0U) {
        return x;
    }

    errno = saved_errno;
    wide = tgamma((double)x);
    core_errno = errno;
    result = (float)wide;

    if (core_errno != saved_errno) {
        return result;
    }

    result_bits = float_bits(result);
    if ((double_bits(wide) & MINI_DOUBLE_EXP) != MINI_DOUBLE_EXP &&
        wide != 0.0 &&
        ((result_bits & MINI_FLOAT_EXP) == 0U ||
         (result_bits & MINI_FLOAT_EXP) == MINI_FLOAT_EXP)) {
        errno = ERANGE;
    } else {
        errno = saved_errno;
    }
    return result;
}
