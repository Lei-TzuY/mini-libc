#include <errno.h>
#include <math.h>
#include <mini/syscall.h>

static unsigned long long dbits(double value)
{
    union {
        double value;
        unsigned long long bits;
    } convert;

    convert.value = value;
    return convert.bits;
}

static unsigned int fbits(float value)
{
    union {
        float value;
        unsigned int bits;
    } convert;

    convert.value = value;
    return convert.bits;
}

static double dfrom(unsigned long long bits)
{
    union {
        double value;
        unsigned long long bits;
    } convert;

    convert.bits = bits;
    return convert.value;
}

static float ffrom(unsigned int bits)
{
    union {
        float value;
        unsigned int bits;
    } convert;

    convert.bits = bits;
    return convert.value;
}

int main(void)
{
    double nan_value = dfrom(0x7ff8000000000042ULL);
    float nan_float = ffrom(0x7fc00042U);
    double result;
    float fresult;

    if (math_errhandling != MATH_ERRNO) {
        return 1;
    }
    if (dbits(fabs(dfrom(0x8000000000000000ULL))) != 0ULL ||
        fbits(fabsf(ffrom(0x80000000U))) != 0U) {
        return 2;
    }
    if (dbits(copysign(1.25, dfrom(0x8000000000000000ULL))) !=
            0xbff4000000000000ULL ||
        fbits(copysignf(1.25f, ffrom(0x80000000U))) != 0xbfa00000U) {
        return 3;
    }
    if (dbits(fmin(0.0, dfrom(0x8000000000000000ULL))) !=
            0x8000000000000000ULL ||
        dbits(fmax(dfrom(0x8000000000000000ULL), 0.0)) != 0ULL) {
        return 4;
    }
    if (fbits(fminf(0.0f, ffrom(0x80000000U))) != 0x80000000U ||
        fbits(fmaxf(ffrom(0x80000000U), 0.0f)) != 0U) {
        return 5;
    }
    if (fmin(nan_value, 2.0) != 2.0 || fmax(2.0, nan_value) != 2.0 ||
        fminf(nan_float, 2.0f) != 2.0f || fmaxf(2.0f, nan_float) != 2.0f) {
        return 6;
    }
    if (trunc(-1.75) != -1.0 || floor(-1.25) != -2.0 ||
        ceil(-1.25) != -1.0 || round(2.5) != 3.0 ||
        round(-2.5) != -3.0) {
        return 7;
    }
    if (truncf(-1.75f) != -1.0f || floorf(-1.25f) != -2.0f ||
        ceilf(-1.25f) != -1.0f || roundf(2.5f) != 3.0f ||
        roundf(-2.5f) != -3.0f) {
        return 8;
    }
    if (dbits(ceil(-0.25)) != 0x8000000000000000ULL ||
        dbits(floor(0.25)) != 0ULL ||
        fbits(ceilf(-0.25f)) != 0x80000000U ||
        fbits(floorf(0.25f)) != 0U) {
        return 9;
    }

    errno = 71;
    result = sqrt(9.0);
    if (result != 3.0 || errno != 71) {
        return 10;
    }
    if (dbits(sqrt(2.0)) != 0x3ff6a09e667f3bcdULL ||
        fbits(sqrtf(2.0f)) != 0x3fb504f3U) {
        return 11;
    }
    if (dbits(sqrt(dfrom(0x8000000000000000ULL))) !=
            0x8000000000000000ULL) {
        return 12;
    }

    errno = 72;
    result = sqrt(-1.0);
    if (result == result || errno != EDOM) {
        return 13;
    }
    errno = 73;
    fresult = sqrtf(-1.0f);
    if (fresult == fresult || errno != EDOM) {
        return 14;
    }

    if (mini_sys_write(1, "math-ok\n", 8) != 8) {
        return 15;
    }
    return 0;
}
