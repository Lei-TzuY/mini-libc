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
    double integral;
    float fintegral;
    int exponent;

    if (math_errhandling != (MATH_ERRNO | MATH_ERREXCEPT)) {
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

    exponent = 99;
    result = frexp(8.0, &exponent);
    if (result != 0.5 || exponent != 4) {
        return 10;
    }
    exponent = 99;
    result = frexp(-6.0, &exponent);
    if (result != -0.75 || exponent != 3) {
        return 11;
    }
    exponent = 99;
    result = frexp(dfrom(1ULL), &exponent);
    if (result != 0.5 || exponent != -1073) {
        return 12;
    }
    exponent = 99;
    fresult = frexpf(ffrom(1U), &exponent);
    if (fresult != 0.5f || exponent != -148) {
        return 13;
    }
    exponent = 99;
    result = frexp(dfrom(0x8000000000000000ULL), &exponent);
    if (dbits(result) != 0x8000000000000000ULL || exponent != 0) {
        return 14;
    }
    exponent = 99;
    result = frexp(nan_value, &exponent);
    if (dbits(result) != dbits(nan_value) || exponent != 0) {
        return 15;
    }

    errno = 81;
    result = scalbn(0.75, 4);
    if (result != 12.0 || errno != 81) {
        return 16;
    }
    errno = 82;
    fresult = ldexpf(0.75f, 4);
    if (fresult != 12.0f || errno != 82) {
        return 17;
    }
    errno = 83;
    result = scalbn(1.0, -1074);
    if (dbits(result) != 1ULL || errno != 83) {
        return 18;
    }
    errno = 84;
    fresult = scalbnf(1.0f, -149);
    if (fbits(fresult) != 1U || errno != 84) {
        return 19;
    }
    errno = 85;
    result = scalbn(dfrom(1ULL), 52);
    if (dbits(result) != 0x0010000000000000ULL || errno != 85) {
        return 20;
    }
    errno = 86;
    result = scalbn(1.5, -1074);
    if (dbits(result) != 2ULL || errno != ERANGE) {
        return 21;
    }
    errno = 87;
    result = scalbn(1.0, -1075);
    if (dbits(result) != 0ULL || errno != ERANGE) {
        return 22;
    }
    errno = 88;
    result = ldexp(-1.0, 1024);
    if (dbits(result) != 0xfff0000000000000ULL || errno != ERANGE) {
        return 23;
    }

    errno = 89;
    result = modf(-3.25, &integral);
    if (result != -0.25 || integral != -3.0 || errno != 89) {
        return 24;
    }
    result = modf(-3.0, &integral);
    if (dbits(result) != 0x8000000000000000ULL || integral != -3.0) {
        return 25;
    }
    fresult = modff(-3.25f, &fintegral);
    if (fresult != -0.25f || fintegral != -3.0f) {
        return 26;
    }
    result = modf(dfrom(0x7ff0000000000000ULL), &integral);
    if (dbits(result) != 0ULL || dbits(integral) != 0x7ff0000000000000ULL) {
        return 27;
    }
    result = modf(nan_value, &integral);
    if (dbits(result) != dbits(nan_value) || dbits(integral) != dbits(nan_value)) {
        return 28;
    }

    errno = 91;
    result = sqrt(9.0);
    if (result != 3.0 || errno != 91) {
        return 29;
    }
    if (dbits(sqrt(2.0)) != 0x3ff6a09e667f3bcdULL ||
        fbits(sqrtf(2.0f)) != 0x3fb504f3U) {
        return 30;
    }
    if (dbits(sqrt(dfrom(0x8000000000000000ULL))) !=
            0x8000000000000000ULL) {
        return 31;
    }

    errno = 92;
    result = sqrt(-1.0);
    if (result == result || errno != EDOM) {
        return 32;
    }
    errno = 93;
    fresult = sqrtf(-1.0f);
    if (fresult == fresult || errno != EDOM) {
        return 33;
    }

    if (mini_sys_write(1, "math-ok\n", 8) != 8) {
        return 34;
    }
    return 0;
}
