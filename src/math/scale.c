#include <errno.h>
#include <fenv.h>
#include <math.h>

#define MINI_DOUBLE_SIGN 0x8000000000000000ULL
#define MINI_DOUBLE_EXP  0x7ff0000000000000ULL
#define MINI_DOUBLE_FRAC 0x000fffffffffffffULL
#define MINI_FLOAT_SIGN 0x80000000U
#define MINI_FLOAT_EXP  0x7f800000U
#define MINI_FLOAT_FRAC 0x007fffffU

enum mini_scale_status {
    MINI_SCALE_EXACT,
    MINI_SCALE_INEXACT,
    MINI_SCALE_OVERFLOW,
    MINI_SCALE_UNDERFLOW
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

static float float_from_bits(unsigned int bits)
{
    union {
        float value;
        unsigned int bits;
    } convert;

    convert.bits = bits;
    return convert.value;
}

static int highest_bit_u64(unsigned long long value)
{
    int bit = -1;

    while (value != 0ULL) {
        ++bit;
        value >>= 1;
    }
    return bit;
}

static unsigned long long round_shift_mode(unsigned long long value,
                                           unsigned long long shift,
                                           int negative, int rounding,
                                           int *discarded)
{
    unsigned long long quotient;
    unsigned long long remainder = 0ULL;

    if (shift == 0ULL) {
        *discarded = 0;
        return value;
    }
    if (shift >= 64ULL) {
        quotient = 0ULL;
        *discarded = value != 0ULL;
    } else {
        quotient = value >> shift;
        remainder = value & ((1ULL << shift) - 1ULL);
        *discarded = remainder != 0ULL;
    }

    if (!*discarded) {
        return quotient;
    }

    if (rounding == FE_UPWARD) {
        if (!negative) {
            ++quotient;
        }
        return quotient;
    }
    if (rounding == FE_DOWNWARD) {
        if (negative) {
            ++quotient;
        }
        return quotient;
    }
    if (rounding == FE_TOWARDZERO) {
        return quotient;
    }

    if (shift < 64ULL) {
        unsigned long long half = 1ULL << (shift - 1ULL);

        if (remainder > half ||
            (remainder == half && (quotient & 1ULL) != 0ULL)) {
            ++quotient;
        }
    }
    return quotient;
}

static double scale_double(double x, int n, int rounding,
                           enum mini_scale_status *status)
{
    unsigned long long bits = double_bits(x);
    unsigned long long sign = bits & MINI_DOUBLE_SIGN;
    unsigned int exponent_field =
        (unsigned int)((bits & MINI_DOUBLE_EXP) >> 52);
    unsigned long long fraction = bits & MINI_DOUBLE_FRAC;
    unsigned long long significand;
    long long exponent;
    long long target;

    *status = MINI_SCALE_EXACT;
    if (exponent_field == 0x7ffU ||
        (exponent_field == 0U && fraction == 0ULL)) {
        return x;
    }

    if (exponent_field != 0U) {
        significand = (1ULL << 52) | fraction;
        exponent = (long long)exponent_field - 1023LL;
    } else {
        int top = highest_bit_u64(fraction);

        significand = fraction << (52 - top);
        exponent = (long long)top - 1074LL;
    }

    target = exponent + (long long)n;
    if (target > 1023LL) {
        *status = MINI_SCALE_OVERFLOW;
        return double_from_bits(sign | MINI_DOUBLE_EXP);
    }
    if (target >= -1022LL) {
        return double_from_bits(sign |
                                ((unsigned long long)(target + 1023LL) << 52) |
                                (significand & MINI_DOUBLE_FRAC));
    }

    {
        unsigned long long shift = (unsigned long long)(-target - 1022LL);
        int discarded;
        unsigned long long rounded =
            round_shift_mode(significand, shift, sign != 0ULL, rounding,
                             &discarded);

        if (rounded >= (1ULL << 52)) {
            if (discarded) {
                *status = MINI_SCALE_INEXACT;
            }
            return double_from_bits(sign | (1ULL << 52));
        }
        if (discarded) {
            *status = MINI_SCALE_UNDERFLOW;
        }
        return double_from_bits(sign | rounded);
    }
}

static float scale_float(float x, int n, int rounding,
                         enum mini_scale_status *status)
{
    unsigned int bits = float_bits(x);
    unsigned int sign = bits & MINI_FLOAT_SIGN;
    unsigned int exponent_field = (bits & MINI_FLOAT_EXP) >> 23;
    unsigned int fraction = bits & MINI_FLOAT_FRAC;
    unsigned long long significand;
    long long exponent;
    long long target;

    *status = MINI_SCALE_EXACT;
    if (exponent_field == 0xffU ||
        (exponent_field == 0U && fraction == 0U)) {
        return x;
    }

    if (exponent_field != 0U) {
        significand = (1ULL << 23) | fraction;
        exponent = (long long)exponent_field - 127LL;
    } else {
        int top = highest_bit_u64(fraction);

        significand = (unsigned long long)fraction << (23 - top);
        exponent = (long long)top - 149LL;
    }

    target = exponent + (long long)n;
    if (target > 127LL) {
        *status = MINI_SCALE_OVERFLOW;
        return float_from_bits(sign | MINI_FLOAT_EXP);
    }
    if (target >= -126LL) {
        return float_from_bits(sign |
                               ((unsigned int)(target + 127LL) << 23) |
                               ((unsigned int)significand & MINI_FLOAT_FRAC));
    }

    {
        unsigned long long shift = (unsigned long long)(-target - 126LL);
        int discarded;
        unsigned long long rounded =
            round_shift_mode(significand, shift, sign != 0U, rounding,
                             &discarded);

        if (rounded >= (1ULL << 23)) {
            if (discarded) {
                *status = MINI_SCALE_INEXACT;
            }
            return float_from_bits(sign | (1U << 23));
        }
        if (discarded) {
            *status = MINI_SCALE_UNDERFLOW;
        }
        return float_from_bits(sign | (unsigned int)rounded);
    }
}

static void finish_scale(const fenv_t *environment, int saved_errno,
                         enum mini_scale_status status)
{
    (void)fesetenv(environment);
    if (status == MINI_SCALE_OVERFLOW) {
        (void)feraiseexcept(FE_OVERFLOW | FE_INEXACT);
        errno = ERANGE;
    } else if (status == MINI_SCALE_UNDERFLOW) {
        (void)feraiseexcept(FE_UNDERFLOW | FE_INEXACT);
        errno = ERANGE;
    } else if (status == MINI_SCALE_INEXACT) {
        (void)feraiseexcept(FE_INEXACT);
        errno = saved_errno;
    } else {
        errno = saved_errno;
    }
}

static double scale_public_double(double x, int n)
{
    fenv_t environment;
    enum mini_scale_status status;
    int saved_errno = errno;
    int rounding = fegetround();
    double result;

    if (rounding < 0) {
        rounding = FE_TONEAREST;
    }
    (void)fegetenv(&environment);
    result = scale_double(x, n, rounding, &status);
    finish_scale(&environment, saved_errno, status);
    return result;
}

static float scale_public_float(float x, int n)
{
    fenv_t environment;
    enum mini_scale_status status;
    int saved_errno = errno;
    int rounding = fegetround();
    float result;

    if (rounding < 0) {
        rounding = FE_TONEAREST;
    }
    (void)fegetenv(&environment);
    result = scale_float(x, n, rounding, &status);
    finish_scale(&environment, saved_errno, status);
    return result;
}

double scalbn(double x, int n)
{
    return scale_public_double(x, n);
}

float scalbnf(float x, int n)
{
    return scale_public_float(x, n);
}

double ldexp(double x, int exp)
{
    return scale_public_double(x, exp);
}

float ldexpf(float x, int exp)
{
    return scale_public_float(x, exp);
}
