#include <errno.h>
#include <fenv.h>
#include <math.h>

#define MINI_DOUBLE_SIGN 0x8000000000000000ULL
#define MINI_DOUBLE_EXP  0x7ff0000000000000ULL
#define MINI_DOUBLE_FRAC 0x000fffffffffffffULL
#define MINI_FLOAT_SIGN  0x80000000U
#define MINI_FLOAT_EXP   0x7f800000U
#define MINI_FLOAT_FRAC  0x007fffffU

static const double erf_num[] = {
    9.60497373987051638749e0,
    9.00260197203842689217e1,
    2.23200534594684319226e3,
    7.00332514112805075473e3,
    5.55923013010394962768e4
};

static const double erf_den[] = {
    3.35617141647503099647e1,
    5.21357949780152679795e2,
    4.59432382970980127987e3,
    2.26290000613805075473e4,
    4.92673942608635921086e4
};

static const double erfc_num[] = {
    2.46196981473530512524e-10,
    5.64189564831068821977e-1,
    7.46321056442269912687e0,
    4.86371970985681366614e1,
    1.96520832956077098242e2,
    5.26445194995477358631e2,
    9.34528527171957607540e2,
    1.02755188689515710272e3,
    5.57535335369399327526e2
};

static const double erfc_den[] = {
    1.32281951154744992508e1,
    8.67072140885989742329e1,
    3.54937778887819891062e2,
    9.75708501743205489753e2,
    1.82390916687909736289e3,
    2.24633760818710981792e3,
    1.65666309194161350182e3,
    5.57535340817727675546e2
};

static const double erfc_tail_num[] = {
    5.64189583547755073984e-1,
    1.27536670759978104416e0,
    5.01905042251180477414e0,
    6.16021097993053585195e0,
    7.40974269950448939160e0,
    2.97886665372100240670e0
};

static const double erfc_tail_den[] = {
    2.26052863220117276590e0,
    9.39603524938001434673e0,
    1.20489539808096656605e1,
    1.70814450747565897222e1,
    9.60896809063285878198e0,
    3.36907645100081516050e0
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

static void finish_public_result(const fenv_t *environment, int result_errno,
                                 int exceptions)
{
    (void)fesetenv(environment);
    errno = result_errno;
    if (exceptions != 0) {
        (void)feraiseexcept(exceptions);
    }
}

static double polynomial(double x, const double *coefficients, unsigned int count)
{
    double value = coefficients[0];
    unsigned int i;

    for (i = 1U; i < count; ++i) {
        value = value * x + coefficients[i];
    }
    return value;
}

static double polynomial_leading_one(double x, const double *coefficients,
                                     unsigned int count)
{
    double value = x + coefficients[0];
    unsigned int i;

    for (i = 1U; i < count; ++i) {
        value = value * x + coefficients[i];
    }
    return value;
}

static double erf_small_positive(double x)
{
    double square = x * x;

    return x * polynomial(square, erf_num, 5U) /
           polynomial_leading_one(square, erf_den, 5U);
}

static double erfc_positive(double x)
{
    const double *numerator;
    const double *denominator;
    unsigned int numerator_count;
    unsigned int denominator_count;
    double exponential;
    double result;
    int saved_errno = errno;

    if (x < 1.0) {
        return 1.0 - erf_small_positive(x);
    }

    if (x < 8.0) {
        numerator = erfc_num;
        denominator = erfc_den;
        numerator_count = 9U;
        denominator_count = 8U;
    } else {
        numerator = erfc_tail_num;
        denominator = erfc_tail_den;
        numerator_count = 6U;
        denominator_count = 6U;
    }

    exponential = exp(-(x * x));
    result = exponential * polynomial(x, numerator, numerator_count) /
             polynomial_leading_one(x, denominator, denominator_count);
    errno = saved_errno;
    return result;
}

double erf(double x)
{
    unsigned long long bits = double_bits(x);
    unsigned long long magnitude_bits = bits & ~MINI_DOUBLE_SIGN;
    double magnitude;
    double result;
    fenv_t environment;
    int saved_errno = errno;

    if (double_is_nan(bits)) {
        return x;
    }
    if (magnitude_bits == MINI_DOUBLE_EXP) {
        return (bits & MINI_DOUBLE_SIGN) != 0ULL ? -1.0 : 1.0;
    }
    if (magnitude_bits == 0ULL) {
        return x;
    }

    (void)fegetenv(&environment);
    magnitude = double_from_bits(magnitude_bits);
    if (magnitude <= 1.0) {
        result = erf_small_positive(magnitude);
    } else if (magnitude >= 6.0) {
        result = 1.0;
    } else {
        result = 1.0 - erfc_positive(magnitude);
    }
    if ((bits & MINI_DOUBLE_SIGN) != 0ULL) {
        result = -result;
    }
    finish_public_result(&environment, saved_errno, FE_INEXACT);
    return result;
}

double erfc(double x)
{
    unsigned long long bits = double_bits(x);
    unsigned long long magnitude_bits = bits & ~MINI_DOUBLE_SIGN;
    double magnitude;
    double result;
    unsigned long long result_bits;
    fenv_t environment;
    int saved_errno = errno;
    int underflow;

    if (double_is_nan(bits)) {
        return x;
    }
    if (magnitude_bits == MINI_DOUBLE_EXP) {
        return (bits & MINI_DOUBLE_SIGN) != 0ULL ? 2.0 : 0.0;
    }
    if (magnitude_bits == 0ULL) {
        return 1.0;
    }

    (void)fegetenv(&environment);
    magnitude = double_from_bits(magnitude_bits);
    result = erfc_positive(magnitude);
    if ((bits & MINI_DOUBLE_SIGN) != 0ULL) {
        result = 2.0 - result;
        finish_public_result(&environment, saved_errno, FE_INEXACT);
        return result;
    }

    result_bits = double_bits(result);
    underflow = (result_bits & MINI_DOUBLE_EXP) == 0ULL;
    finish_public_result(&environment, underflow ? ERANGE : saved_errno,
                         underflow ? FE_UNDERFLOW | FE_INEXACT : FE_INEXACT);
    return result;
}

float erff(float x)
{
    unsigned int bits = float_bits(x);
    unsigned int magnitude_bits = bits & ~MINI_FLOAT_SIGN;
    float result;
    fenv_t environment;
    int saved_errno = errno;

    if (float_is_nan(bits)) {
        return x;
    }
    if (magnitude_bits == MINI_FLOAT_EXP) {
        return (bits & MINI_FLOAT_SIGN) != 0U ? -1.0f : 1.0f;
    }
    if (magnitude_bits == 0U) {
        return x;
    }

    (void)fegetenv(&environment);
    result = (float)erf((double)x);
    finish_public_result(&environment, saved_errno, FE_INEXACT);
    return result;
}

float erfcf(float x)
{
    unsigned int bits = float_bits(x);
    unsigned int magnitude_bits = bits & ~MINI_FLOAT_SIGN;
    float result;
    unsigned int result_bits;
    fenv_t environment;
    int saved_errno = errno;
    int underflow;

    if (float_is_nan(bits)) {
        return x;
    }
    if (magnitude_bits == MINI_FLOAT_EXP) {
        return (bits & MINI_FLOAT_SIGN) != 0U ? 2.0f : 0.0f;
    }
    if (magnitude_bits == 0U) {
        return 1.0f;
    }

    (void)fegetenv(&environment);
    result = (float)erfc((double)x);
    result_bits = float_bits(result);
    underflow = (bits & MINI_FLOAT_SIGN) == 0U &&
                (result_bits & MINI_FLOAT_EXP) == 0U;
    finish_public_result(&environment, underflow ? ERANGE : saved_errno,
                         underflow ? FE_UNDERFLOW | FE_INEXACT : FE_INEXACT);
    return result;
}
