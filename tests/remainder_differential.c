#include <math.h>
#include <stdio.h>

extern int __mini_fpclassify(double x);
extern int __mini_fpclassifyf(float x);
extern int __mini_signbit(double x);
extern int __mini_signbitf(float x);
extern int __mini_math_compare(double x, double y, int operation);
extern int __mini_math_comparef(float x, float y, int operation);
extern double mini_test_fmod(double x, double y);
extern float mini_test_fmodf(float x, float y);
extern double mini_test_remainder(double x, double y);
extern float mini_test_remainderf(float x, float y);
extern double mini_test_remquo(double x, double y, int *quo);
extern float mini_test_remquof(float x, float y, int *quo);

#define MINI_FP_NAN 0
#define MINI_FP_INFINITE 1
#define MINI_FP_ZERO 2
#define MINI_FP_SUBNORMAL 3
#define MINI_FP_NORMAL 4

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

static int classification_matches(double value)
{
    int host = fpclassify(value);
    int mini = __mini_fpclassify(value);

    if (host == FP_NAN) {
        return mini == MINI_FP_NAN;
    }
    if (host == FP_INFINITE) {
        return mini == MINI_FP_INFINITE;
    }
    if (host == FP_ZERO) {
        return mini == MINI_FP_ZERO;
    }
    if (host == FP_SUBNORMAL) {
        return mini == MINI_FP_SUBNORMAL;
    }
    return mini == MINI_FP_NORMAL;
}

static int classification_matchesf(float value)
{
    int host = fpclassify(value);
    int mini = __mini_fpclassifyf(value);

    if (host == FP_NAN) {
        return mini == MINI_FP_NAN;
    }
    if (host == FP_INFINITE) {
        return mini == MINI_FP_INFINITE;
    }
    if (host == FP_ZERO) {
        return mini == MINI_FP_ZERO;
    }
    if (host == FP_SUBNORMAL) {
        return mini == MINI_FP_SUBNORMAL;
    }
    return mini == MINI_FP_NORMAL;
}

static int quotient_bits_match(int mini, int host)
{
    unsigned int mini_magnitude = (unsigned int)(mini < 0 ? -mini : mini);
    unsigned int host_magnitude = (unsigned int)(host < 0 ? -host : host);

    if ((mini_magnitude & 7U) != (host_magnitude & 7U)) {
        return 0;
    }
    if ((mini_magnitude & 7U) != 0U && ((mini < 0) != (host < 0))) {
        return 0;
    }
    return 1;
}

int main(void)
{
    double special_values[] = {
        0.0,
        -0.0,
        dfrom(0x0000000000000001ULL),
        dfrom(0x0010000000000000ULL),
        1.0,
        -2.0,
        dfrom(0x7ff0000000000000ULL),
        dfrom(0xfff0000000000000ULL),
        dfrom(0x7ff8000000000042ULL)
    };
    float special_valuesf[] = {
        0.0f,
        -0.0f,
        ffrom(0x00000001U),
        ffrom(0x00800000U),
        1.0f,
        -2.0f,
        ffrom(0x7f800000U),
        ffrom(0xff800000U),
        ffrom(0x7fc01234U)
    };
    double xs[] = {
        13.0, -13.0, 5.3, -5.3, 6.0, 10.0, 29.0, 30.0,
        0x1p900, -0x1p900, 0x1.fffffffffffffp500, 0x1.0000000000001p-500,
        dfrom(0x0010000000000001ULL), dfrom(0x0000000000000003ULL)
    };
    double ys[] = {
        4.0, -4.0, 2.0, 3.0, 5.0, 0.75, 0x1p-400,
        dfrom(0x0010000000000000ULL), dfrom(0x0000000000000001ULL)
    };
    float xsf[] = {
        13.0f, -13.0f, 5.3f, -5.3f, 6.0f, 10.0f, 29.0f, 30.0f,
        0x1p100f, -0x1p100f, ffrom(0x00800001U), ffrom(0x00000003U)
    };
    float ysf[] = {
        4.0f, -4.0f, 2.0f, 3.0f, 5.0f, 0.75f, 0x1p-80f,
        ffrom(0x00800000U), ffrom(0x00000001U)
    };
    size_t i;
    size_t j;

    for (i = 0; i < sizeof(special_values) / sizeof(special_values[0]); ++i) {
        double value = special_values[i];

        if (!classification_matches(value) ||
            (__mini_signbit(value) != 0) != (signbit(value) != 0)) {
            fprintf(stderr, "double classification mismatch at %zu\n", i);
            return 1;
        }
    }
    for (i = 0; i < sizeof(special_valuesf) / sizeof(special_valuesf[0]); ++i) {
        float value = special_valuesf[i];

        if (!classification_matchesf(value) ||
            (__mini_signbitf(value) != 0) != (signbit(value) != 0)) {
            fprintf(stderr, "float classification mismatch at %zu\n", i);
            return 2;
        }
    }

    {
        double nan_value = dfrom(0x7ff8000000000001ULL);
        float fnan = ffrom(0x7fc00001U);

        if (!__mini_math_compare(nan_value, 1.0, 0) ||
            __mini_math_compare(nan_value, 1.0, 1) ||
            !__mini_math_compare(2.0, 1.0, 1) ||
            !__mini_math_compare(2.0, 2.0, 2) ||
            !__mini_math_compare(-1.0, 0.0, 3) ||
            !__mini_math_compare(1.0, 1.0, 4) ||
            !__mini_math_compare(1.0, 2.0, 5) ||
            !__mini_math_comparef(fnan, 1.0f, 0) ||
            __mini_math_comparef(fnan, 1.0f, 3)) {
            fprintf(stderr, "ordered comparison mismatch\n");
            return 3;
        }
    }

    for (i = 0; i < sizeof(xs) / sizeof(xs[0]); ++i) {
        for (j = 0; j < sizeof(ys) / sizeof(ys[0]); ++j) {
            double x = xs[i];
            double y = ys[j];
            double mini_fmod = mini_test_fmod(x, y);
            double host_fmod = fmod(x, y);
            double mini_remainder = mini_test_remainder(x, y);
            double host_remainder = remainder(x, y);
            int mini_quo = 0;
            int host_quo = 0;
            double mini_remquo = mini_test_remquo(x, y, &mini_quo);
            double host_remquo = remquo(x, y, &host_quo);

            if (dbits(mini_fmod) != dbits(host_fmod) ||
                dbits(mini_remainder) != dbits(host_remainder) ||
                dbits(mini_remquo) != dbits(host_remquo) ||
                !quotient_bits_match(mini_quo, host_quo)) {
                fprintf(stderr, "double remainder mismatch at %zu,%zu\n", i, j);
                return 4;
            }
        }
    }

    for (i = 0; i < sizeof(xsf) / sizeof(xsf[0]); ++i) {
        for (j = 0; j < sizeof(ysf) / sizeof(ysf[0]); ++j) {
            float x = xsf[i];
            float y = ysf[j];
            float mini_fmod = mini_test_fmodf(x, y);
            float host_fmod = fmodf(x, y);
            float mini_remainder = mini_test_remainderf(x, y);
            float host_remainder = remainderf(x, y);
            int mini_quo = 0;
            int host_quo = 0;
            float mini_remquo = mini_test_remquof(x, y, &mini_quo);
            float host_remquo = remquof(x, y, &host_quo);

            if (fbits(mini_fmod) != fbits(host_fmod) ||
                fbits(mini_remainder) != fbits(host_remainder) ||
                fbits(mini_remquo) != fbits(host_remquo) ||
                !quotient_bits_match(mini_quo, host_quo)) {
                fprintf(stderr, "float remainder mismatch at %zu,%zu\n", i, j);
                return 5;
            }
        }
    }

    puts("remainder differential passed");
    return 0;
}
