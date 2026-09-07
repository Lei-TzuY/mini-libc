#include <errno.h>
#include <math.h>

#define MINI_DOUBLE_SIGN 0x8000000000000000ULL
#define MINI_DOUBLE_EXP  0x7ff0000000000000ULL
#define MINI_DOUBLE_FRAC 0x000fffffffffffffULL
#define MINI_FLOAT_EXP   0x7f800000U
#define MINI_FLOAT_FRAC  0x007fffffU

struct mini_integer_info {
    int is_integer;
    int odd;
    int fits_ull;
    unsigned long long magnitude;
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

static struct mini_integer_info classify_integer(double value)
{
    struct mini_integer_info info;
    unsigned long long bits = double_bits(value);
    unsigned long long magnitude_bits = bits & ~MINI_DOUBLE_SIGN;
    unsigned long long fraction = bits & MINI_DOUBLE_FRAC;
    unsigned long long significand;
    unsigned int exponent_field;
    int exponent;

    info.is_integer = 0;
    info.odd = 0;
    info.fits_ull = 0;
    info.magnitude = 0ULL;

    if (magnitude_bits == 0ULL) {
        info.is_integer = 1;
        info.fits_ull = 1;
        return info;
    }

    exponent_field = (unsigned int)(magnitude_bits >> 52);
    if (exponent_field == 0U || exponent_field == 0x7ffU) {
        return info;
    }
    exponent = (int)exponent_field - 1023;
    if (exponent < 0) {
        return info;
    }

    significand = (1ULL << 52) | fraction;
    if (exponent < 52) {
        unsigned int shift = (unsigned int)(52 - exponent);
        unsigned long long mask = (1ULL << shift) - 1ULL;

        if ((significand & mask) != 0ULL) {
            return info;
        }
        info.magnitude = significand >> shift;
        info.is_integer = 1;
        info.odd = (info.magnitude & 1ULL) != 0ULL;
        info.fits_ull = 1;
        return info;
    }

    info.is_integer = 1;
    if (exponent == 52) {
        info.odd = (significand & 1ULL) != 0ULL;
    }
    if (exponent < 64) {
        info.magnitude = significand << (unsigned int)(exponent - 52);
        info.fits_ull = 1;
    }
    return info;
}

static double integer_power(double base, unsigned long long exponent)
{
    double result = 1.0;
    double factor = base;

    while (exponent != 0ULL) {
        if ((exponent & 1ULL) != 0ULL) {
            result *= factor;
        }
        exponent >>= 1;
        if (exponent != 0ULL) {
            factor *= factor;
        }
    }
    return result;
}

static double apply_sign(double value, int negative)
{
    unsigned long long bits;

    if (!negative) {
        return value;
    }
    bits = double_bits(value) | MINI_DOUBLE_SIGN;
    return double_from_bits(bits);
}

static double finish_finite(double result, int negative)
{
    unsigned long long bits;
    unsigned long long magnitude;

    result = apply_sign(result, negative);
    bits = double_bits(result);
    magnitude = bits & ~MINI_DOUBLE_SIGN;
    if ((magnitude & MINI_DOUBLE_EXP) == MINI_DOUBLE_EXP ||
        (magnitude & MINI_DOUBLE_EXP) == 0ULL) {
        errno = ERANGE;
    }
    return result;
}

double pow(double x, double y)
{
    unsigned long long xbits = double_bits(x);
    unsigned long long ybits = double_bits(y);
    unsigned long long xmag = xbits & ~MINI_DOUBLE_SIGN;
    unsigned long long ymag = ybits & ~MINI_DOUBLE_SIGN;
    struct mini_integer_info yint;
    double ax = double_from_bits(xmag);
    double result;
    int negative_result = 0;

    if (ymag == 0ULL) {
        return 1.0;
    }
    if (xbits == 0x3ff0000000000000ULL) {
        return 1.0;
    }
    if (xbits == 0xbff0000000000000ULL && ymag == MINI_DOUBLE_EXP) {
        return 1.0;
    }
    if (double_is_nan(ybits)) {
        return y;
    }
    if (double_is_nan(xbits)) {
        return x;
    }

    if (ymag == MINI_DOUBLE_EXP) {
        if (ax == 1.0) {
            return 1.0;
        }
        if ((ybits & MINI_DOUBLE_SIGN) == 0ULL) {
            return ax > 1.0 ? double_from_bits(MINI_DOUBLE_EXP) : 0.0;
        }
        return ax > 1.0 ? 0.0 : double_from_bits(MINI_DOUBLE_EXP);
    }

    yint = classify_integer(y);

    if (xmag == 0ULL) {
        negative_result = (xbits & MINI_DOUBLE_SIGN) != 0ULL &&
                          yint.is_integer && yint.odd;
        if (y > 0.0) {
            return apply_sign(0.0, negative_result);
        }
        errno = ERANGE;
        return apply_sign(double_from_bits(MINI_DOUBLE_EXP), negative_result);
    }

    if (xmag == MINI_DOUBLE_EXP) {
        negative_result = (xbits & MINI_DOUBLE_SIGN) != 0ULL &&
                          yint.is_integer && yint.odd;
        if (y > 0.0) {
            return apply_sign(double_from_bits(MINI_DOUBLE_EXP), negative_result);
        }
        return apply_sign(0.0, negative_result);
    }

    if ((xbits & MINI_DOUBLE_SIGN) != 0ULL) {
        if (!yint.is_integer) {
            errno = EDOM;
            return double_from_bits(0x7ff8000000000000ULL);
        }
        negative_result = yint.odd;
    }

    if (yint.is_integer && yint.fits_ull) {
        double base = ax;

        if (y < 0.0) {
            base = 1.0 / base;
        }
        result = integer_power(base, yint.magnitude);
        if (result == 1.0 && ax == 1.0) {
            return apply_sign(result, negative_result);
        }
        return finish_finite(result, negative_result);
    }

    result = exp(y * log(ax));
    return finish_finite(result, negative_result);
}

float powf(float x, float y)
{
    unsigned int xbits = float_bits(x);
    unsigned int ybits = float_bits(y);
    unsigned int xmag = xbits & 0x7fffffffU;
    unsigned int ymag = ybits & 0x7fffffffU;
    double wide;
    float result;
    unsigned int result_bits;

    if ((ybits & MINI_FLOAT_EXP) == MINI_FLOAT_EXP &&
        (ybits & MINI_FLOAT_FRAC) != 0U) {
        if (x == 1.0f) {
            return 1.0f;
        }
        return y;
    }
    if ((xbits & MINI_FLOAT_EXP) == MINI_FLOAT_EXP &&
        (xbits & MINI_FLOAT_FRAC) != 0U) {
        if (ymag == 0U) {
            return 1.0f;
        }
        return x;
    }
    if (xmag == 0x3f800000U && ymag == MINI_FLOAT_EXP) {
        return 1.0f;
    }

    wide = pow((double)x, (double)y);
    result = (float)wide;
    result_bits = float_bits(result);

    if ((xbits & MINI_FLOAT_EXP) != MINI_FLOAT_EXP &&
        (ybits & MINI_FLOAT_EXP) != MINI_FLOAT_EXP &&
        xmag != 0U &&
        ((result_bits & MINI_FLOAT_EXP) == MINI_FLOAT_EXP ||
         (result_bits & MINI_FLOAT_EXP) == 0U)) {
        errno = ERANGE;
    }
    return result;
}
