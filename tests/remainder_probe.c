#include <errno.h>
#include <math.h>
#include <mini/syscall.h>

static double dfrom(unsigned long long bits)
{
    union {
        double value;
        unsigned long long bits;
    } convert;

    convert.bits = bits;
    return convert.value;
}

static unsigned long long dbits(double value)
{
    union {
        double value;
        unsigned long long bits;
    } convert;

    convert.value = value;
    return convert.bits;
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

static unsigned int fbits(float value)
{
    union {
        float value;
        unsigned int bits;
    } convert;

    convert.value = value;
    return convert.bits;
}

int main(void)
{
    double inf = dfrom(0x7ff0000000000000ULL);
    double nan_value = dfrom(0x7ff8000000000042ULL);
    double dsub = dfrom(0x0000000000000001ULL);
    float fsub = ffrom(0x00000001U);
    float fnan = ffrom(0x7fc01234U);
    double dvalues[2] = {1.0, 2.0};
    float fvalues[2] = {1.0f, 2.0f};
    int left_index;
    int right_index;
    int quo;
    double value;

    if (fpclassify(0.0) != FP_ZERO || fpclassify(-0.0) != FP_ZERO ||
        fpclassify(dsub) != FP_SUBNORMAL || fpclassify(1.0) != FP_NORMAL ||
        fpclassify(inf) != FP_INFINITE || fpclassify(nan_value) != FP_NAN) {
        return 1;
    }
    if (fpclassify(fsub) != FP_SUBNORMAL || fpclassify(1.0f) != FP_NORMAL ||
        fpclassify(fnan) != FP_NAN) {
        return 2;
    }
    if (!isfinite(dsub) || isfinite(inf) || !isinf(inf) || !isnan(nan_value) ||
        isnormal(dsub) || !isnormal(1.0) || !signbit(-0.0) || signbit(0.0) ||
        !signbit(ffrom(0x80000000U))) {
        return 3;
    }

    left_index = 0;
    if (!isfinite(dvalues[left_index++]) || left_index != 1) {
        return 4;
    }
    left_index = 0;
    if (!isnormal(fvalues[left_index++]) || left_index != 1) {
        return 5;
    }
    left_index = 0;
    right_index = 0;
    if (!isless(dvalues[left_index++], dvalues[right_index++ + 1]) ||
        left_index != 1 || right_index != 1) {
        return 6;
    }

    if (!isunordered(nan_value, 1.0) || isgreater(nan_value, 1.0) ||
        !isgreater(2.0, 1.0f) || !isgreaterequal(2.0f, 2.0) ||
        !isless(-1.0f, 0.0f) || !islessequal(1.0, 1.0) ||
        !islessgreater(1.0f, 2.0f)) {
        return 7;
    }

    errno = 41;
    if (fmod(13.0, 4.0) != 1.0 || fmod(-13.0, 4.0) != -1.0 ||
        fmod(scalbn(1.0, 900), 3.0) != 1.0 || errno != 41) {
        return 8;
    }
    if (dbits(fmod(-0.0, 3.0)) != 0x8000000000000000ULL ||
        fmod(3.0, inf) != 3.0 || errno != 41) {
        return 9;
    }

    if (remainder(6.0, 4.0) != -2.0 || remainder(10.0, 4.0) != 2.0 ||
        remainder(-6.0, 4.0) != 2.0 || errno != 41) {
        return 10;
    }
    quo = 99;
    if (remquo(30.0, 4.0, &quo) != -2.0 || quo != 8) {
        return 11;
    }
    quo = 99;
    if (remquo(-30.0, 4.0, &quo) != 2.0 || quo != -8) {
        return 12;
    }
    quo = 99;
    if (remquo(scalbn(1.0, 900), 3.0, &quo) != 1.0 || quo != 85) {
        return 13;
    }

    if (fmodf(13.0f, 4.0f) != 1.0f || remainderf(6.0f, 4.0f) != -2.0f) {
        return 14;
    }
    quo = 99;
    if (remquof(-30.0f, -4.0f, &quo) != 2.0f || quo != 8) {
        return 15;
    }
    if (fbits(fmodf(-0.0f, 3.0f)) != 0x80000000U) {
        return 16;
    }

    errno = 42;
    value = fmod(inf, 1.0);
    if (!isnan(value) || errno != EDOM) {
        return 17;
    }
    errno = 43;
    value = remainder(1.0, 0.0);
    if (!isnan(value) || errno != EDOM) {
        return 18;
    }
    errno = 44;
    quo = 99;
    value = remquo(inf, 2.0, &quo);
    if (!isnan(value) || errno != EDOM || quo != 0) {
        return 19;
    }

    errno = 45;
    if (dbits(fmod(nan_value, 2.0)) != 0x7ff8000000000042ULL || errno != 45 ||
        fbits(remainderf(fnan, 2.0f)) != 0x7fc01234U || errno != 45) {
        return 20;
    }

    if (mini_sys_write(1, "remainder-ok\n", 13) != 13) {
        return 21;
    }
    return 0;
}
