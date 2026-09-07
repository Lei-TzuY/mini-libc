#include <errno.h>
#include <fenv.h>
#include <math.h>

#define MINI_DOUBLE_SIGN 0x8000000000000000ULL
#define MINI_DOUBLE_EXP  0x7ff0000000000000ULL
#define MINI_DOUBLE_FRAC 0x000fffffffffffffULL
#define MINI_FLOAT_SIGN 0x80000000U
#define MINI_FLOAT_EXP  0x7f800000U
#define MINI_FLOAT_FRAC 0x007fffffU

#define MINI_LONG_MIN (-9223372036854775807L - 1L)
#define MINI_LLONG_MIN (-9223372036854775807LL - 1LL)
#define MINI_INT64_LIMIT 9223372036854775808.0

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

static int mode_valid(int mode)
{
    return mode == FE_TONEAREST || mode == FE_DOWNWARD ||
           mode == FE_UPWARD || mode == FE_TOWARDZERO;
}

static double round_double_mode(double x, int mode, int *inexact)
{
    unsigned long long bits = double_bits(x);
    unsigned long long sign = bits & MINI_DOUBLE_SIGN;
    unsigned long long magnitude = bits & ~MINI_DOUBLE_SIGN;
    unsigned int exponent_field = (unsigned int)((bits & MINI_DOUBLE_EXP) >> 52);
    int exponent;
    unsigned int fractional_bits;
    unsigned long long unit;
    unsigned long long mask;
    unsigned long long discarded;
    unsigned long long rounded;

    *inexact = 0;
    if (exponent_field == 0x7ffU || magnitude == 0ULL) {
        return x;
    }

    exponent = exponent_field == 0U ? -1022 : (int)exponent_field - 1023;
    if (exponent < 0) {
        *inexact = 1;
        if (mode == FE_UPWARD && sign == 0ULL) {
            return 1.0;
        }
        if (mode == FE_DOWNWARD && sign != 0ULL) {
            return -1.0;
        }
        if (mode == FE_TONEAREST) {
            const unsigned long long half = 0x3fe0000000000000ULL;

            if (magnitude > half) {
                return sign != 0ULL ? -1.0 : 1.0;
            }
        }
        return double_from_bits(sign);
    }
    if (exponent >= 52) {
        return x;
    }

    fractional_bits = 52U - (unsigned int)exponent;
    unit = 1ULL << fractional_bits;
    mask = unit - 1ULL;
    discarded = magnitude & mask;
    if (discarded == 0ULL) {
        return x;
    }

    *inexact = 1;
    rounded = magnitude & ~mask;
    if ((mode == FE_UPWARD && sign == 0ULL) ||
        (mode == FE_DOWNWARD && sign != 0ULL)) {
        rounded += unit;
    } else if (mode == FE_TONEAREST) {
        unsigned long long half = unit >> 1;

        if (discarded > half ||
            (discarded == half && (rounded & unit) != 0ULL)) {
            rounded += unit;
        }
    }
    return double_from_bits(sign | rounded);
}

static float round_float_mode(float x, int mode, int *inexact)
{
    unsigned int bits = float_bits(x);
    unsigned int sign = bits & MINI_FLOAT_SIGN;
    unsigned int magnitude = bits & ~MINI_FLOAT_SIGN;
    unsigned int exponent_field = (bits & MINI_FLOAT_EXP) >> 23;
    int exponent;
    unsigned int fractional_bits;
    unsigned int unit;
    unsigned int mask;
    unsigned int discarded;
    unsigned int rounded;

    *inexact = 0;
    if (exponent_field == 0xffU || magnitude == 0U) {
        return x;
    }

    exponent = exponent_field == 0U ? -126 : (int)exponent_field - 127;
    if (exponent < 0) {
        *inexact = 1;
        if (mode == FE_UPWARD && sign == 0U) {
            return 1.0f;
        }
        if (mode == FE_DOWNWARD && sign != 0U) {
            return -1.0f;
        }
        if (mode == FE_TONEAREST) {
            const unsigned int half = 0x3f000000U;

            if (magnitude > half) {
                return sign != 0U ? -1.0f : 1.0f;
            }
        }
        return float_from_bits(sign);
    }
    if (exponent >= 23) {
        return x;
    }

    fractional_bits = 23U - (unsigned int)exponent;
    unit = 1U << fractional_bits;
    mask = unit - 1U;
    discarded = magnitude & mask;
    if (discarded == 0U) {
        return x;
    }

    *inexact = 1;
    rounded = magnitude & ~mask;
    if ((mode == FE_UPWARD && sign == 0U) ||
        (mode == FE_DOWNWARD && sign != 0U)) {
        rounded += unit;
    } else if (mode == FE_TONEAREST) {
        unsigned int half = unit >> 1;

        if (discarded > half ||
            (discarded == half && (rounded & unit) != 0U)) {
            rounded += unit;
        }
    }
    return float_from_bits(sign | rounded);
}

static int environment_failure(void)
{
    errno = EDOM;
    (void)feraiseexcept(FE_INVALID);
    return 0;
}

static int conversion_failure(int range_error)
{
    errno = range_error ? ERANGE : EDOM;
    (void)feraiseexcept(FE_INVALID);
    return 0;
}

double rint(double x)
{
    int inexact;
    int mode = fegetround();
    double result;

    if (!mode_valid(mode)) {
        (void)environment_failure();
        return x;
    }
    result = round_double_mode(x, mode, &inexact);
    if (inexact) {
        (void)feraiseexcept(FE_INEXACT);
    }
    return result;
}

float rintf(float x)
{
    int inexact;
    int mode = fegetround();
    float result;

    if (!mode_valid(mode)) {
        (void)environment_failure();
        return x;
    }
    result = round_float_mode(x, mode, &inexact);
    if (inexact) {
        (void)feraiseexcept(FE_INEXACT);
    }
    return result;
}

double nearbyint(double x)
{
    int inexact;
    int mode = fegetround();

    if (!mode_valid(mode)) {
        (void)environment_failure();
        return x;
    }
    return round_double_mode(x, mode, &inexact);
}

float nearbyintf(float x)
{
    int inexact;
    int mode = fegetround();

    if (!mode_valid(mode)) {
        (void)environment_failure();
        return x;
    }
    return round_float_mode(x, mode, &inexact);
}

static int rounded_double_to_i64(double x, long long *result, int *inexact)
{
    unsigned long long bits = double_bits(x);
    int mode = fegetround();
    double rounded;

    if (!mode_valid(mode)) {
        return environment_failure();
    }
    if ((bits & MINI_DOUBLE_EXP) == MINI_DOUBLE_EXP) {
        return conversion_failure(0);
    }

    rounded = round_double_mode(x, mode, inexact);
    if (rounded >= MINI_INT64_LIMIT || rounded < -MINI_INT64_LIMIT) {
        return conversion_failure(1);
    }
    *result = (long long)rounded;
    if (*inexact) {
        (void)feraiseexcept(FE_INEXACT);
    }
    return 1;
}

static int rounded_float_to_i64(float x, long long *result, int *inexact)
{
    unsigned int bits = float_bits(x);
    int mode = fegetround();
    float rounded;
    double widened;

    if (!mode_valid(mode)) {
        return environment_failure();
    }
    if ((bits & MINI_FLOAT_EXP) == MINI_FLOAT_EXP) {
        return conversion_failure(0);
    }

    rounded = round_float_mode(x, mode, inexact);
    widened = (double)rounded;
    if (widened >= MINI_INT64_LIMIT || widened < -MINI_INT64_LIMIT) {
        return conversion_failure(1);
    }
    *result = (long long)rounded;
    if (*inexact) {
        (void)feraiseexcept(FE_INEXACT);
    }
    return 1;
}

long lrint(double x)
{
    long long result;
    int inexact;

    if (!rounded_double_to_i64(x, &result, &inexact)) {
        return MINI_LONG_MIN;
    }
    return (long)result;
}

long lrintf(float x)
{
    long long result;
    int inexact;

    if (!rounded_float_to_i64(x, &result, &inexact)) {
        return MINI_LONG_MIN;
    }
    return (long)result;
}

long long llrint(double x)
{
    long long result;
    int inexact;

    if (!rounded_double_to_i64(x, &result, &inexact)) {
        return MINI_LLONG_MIN;
    }
    return result;
}

long long llrintf(float x)
{
    long long result;
    int inexact;

    if (!rounded_float_to_i64(x, &result, &inexact)) {
        return MINI_LLONG_MIN;
    }
    return result;
}
