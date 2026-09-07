#include <errno.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>

extern double mini_test_fabs(double x);
extern float mini_test_fabsf(float x);
extern double mini_test_copysign(double x, double y);
extern float mini_test_copysignf(float x, float y);
extern double mini_test_fmin(double x, double y);
extern float mini_test_fminf(float x, float y);
extern double mini_test_fmax(double x, double y);
extern float mini_test_fmaxf(float x, float y);
extern double mini_test_trunc(double x);
extern float mini_test_truncf(float x);
extern double mini_test_floor(double x);
extern float mini_test_floorf(float x);
extern double mini_test_ceil(double x);
extern float mini_test_ceilf(float x);
extern double mini_test_round(double x);
extern float mini_test_roundf(float x);
extern double mini_test_frexp(double x, int *exp);
extern float mini_test_frexpf(float x, int *exp);
extern double mini_test_ldexp(double x, int exp);
extern float mini_test_ldexpf(float x, int exp);
extern double mini_test_scalbn(double x, int n);
extern float mini_test_scalbnf(float x, int n);
extern double mini_test_modf(double x, double *iptr);
extern float mini_test_modff(float x, float *iptr);
extern double mini_test_sqrt(double x);
extern float mini_test_sqrtf(float x);
extern int *__mini_errno_location(void);

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

static int dnan(double value)
{
    unsigned long long bits = dbits(value);
    return (bits & 0x7ff0000000000000ULL) == 0x7ff0000000000000ULL &&
           (bits & 0x000fffffffffffffULL) != 0ULL;
}

static int fnan(float value)
{
    unsigned int bits = fbits(value);
    return (bits & 0x7f800000U) == 0x7f800000U &&
           (bits & 0x007fffffU) != 0U;
}

static int same_double(double left, double right)
{
    return dbits(left) == dbits(right) || (dnan(left) && dnan(right));
}

static int same_float(float left, float right)
{
    return fbits(left) == fbits(right) || (fnan(left) && fnan(right));
}

int main(void)
{
    static const double values[] = {
        -4503599627370495.5, -17.75, -2.5, -1.5, -0.5, -0.25,
        -0.0, 0.0, 0.25, 0.5, 1.5, 2.5, 17.75, 4503599627370495.5
    };
    static const float fvalues[] = {
        -8388607.5f, -17.75f, -2.5f, -1.5f, -0.5f, -0.25f,
        -0.0f, 0.0f, 0.25f, 0.5f, 1.5f, 2.5f, 17.75f, 8388607.5f
    };
    static const double roots[] = {0.0, 0.25, 0.5, 1.0, 2.0, 9.0, 65536.0};
    static const float froots[] = {0.0f, 0.25f, 0.5f, 1.0f, 2.0f, 9.0f, 65536.0f};
    static const double decompose_values[] = {
        -17.75, -1.5, -0.0, 0.0, 0.5, 1.0, 17.75,
        0x1p-1022, 0x0.0000000000001p-1022, 0x0.fffffffffffffp-1022
    };
    static const float fdecompose_values[] = {
        -17.75f, -1.5f, -0.0f, 0.0f, 0.5f, 1.0f, 17.75f,
        0x1p-126f, 0x0.000002p-126f, 0x0.fffffep-126f
    };
    static const struct {
        double value;
        int exponent;
    } scale_cases[] = {
        {0.75, 4}, {-0.75, 4}, {1.0, -1022}, {1.0, -1074},
        {1.5, -1074}, {1.0, -1075}, {0x0.0000000000001p-1022, 52}
    };
    static const struct {
        float value;
        int exponent;
    } fscale_cases[] = {
        {0.75f, 4}, {-0.75f, 4}, {1.0f, -126}, {1.0f, -149},
        {1.5f, -149}, {1.0f, -150}, {0x0.000002p-126f, 23}
    };
    double qnan = dfrom(0x7ff8000000001234ULL);
    float fqnan = ffrom(0x7fc01234U);
    size_t i;

    for (i = 0; i < sizeof(values) / sizeof(values[0]); ++i) {
        double x = values[i];

        if (!same_double(mini_test_fabs(x), fabs(x)) ||
            !same_double(mini_test_trunc(x), trunc(x)) ||
            !same_double(mini_test_floor(x), floor(x)) ||
            !same_double(mini_test_ceil(x), ceil(x)) ||
            !same_double(mini_test_round(x), round(x))) {
            fprintf(stderr, "double transform mismatch at index %zu\n", i);
            return 1;
        }
    }

    for (i = 0; i < sizeof(fvalues) / sizeof(fvalues[0]); ++i) {
        float x = fvalues[i];

        if (!same_float(mini_test_fabsf(x), fabsf(x)) ||
            !same_float(mini_test_truncf(x), truncf(x)) ||
            !same_float(mini_test_floorf(x), floorf(x)) ||
            !same_float(mini_test_ceilf(x), ceilf(x)) ||
            !same_float(mini_test_roundf(x), roundf(x))) {
            fprintf(stderr, "float transform mismatch at index %zu\n", i);
            return 2;
        }
    }

    if (!same_double(mini_test_copysign(1.25, -0.0), copysign(1.25, -0.0)) ||
        !same_double(mini_test_copysign(qnan, -1.0), copysign(qnan, -1.0)) ||
        !same_float(mini_test_copysignf(1.25f, -0.0f), copysignf(1.25f, -0.0f)) ||
        !same_float(mini_test_copysignf(fqnan, -1.0f), copysignf(fqnan, -1.0f))) {
        return 3;
    }

    if (!same_double(mini_test_fmin(0.0, -0.0), fmin(0.0, -0.0)) ||
        !same_double(mini_test_fmax(-0.0, 0.0), fmax(-0.0, 0.0)) ||
        !same_double(mini_test_fmin(qnan, 7.0), fmin(qnan, 7.0)) ||
        !same_double(mini_test_fmax(7.0, qnan), fmax(7.0, qnan)) ||
        !same_float(mini_test_fminf(0.0f, -0.0f), fminf(0.0f, -0.0f)) ||
        !same_float(mini_test_fmaxf(-0.0f, 0.0f), fmaxf(-0.0f, 0.0f)) ||
        !same_float(mini_test_fminf(fqnan, 7.0f), fminf(fqnan, 7.0f)) ||
        !same_float(mini_test_fmaxf(7.0f, fqnan), fmaxf(7.0f, fqnan))) {
        return 4;
    }

    for (i = 0; i < sizeof(decompose_values) / sizeof(decompose_values[0]); ++i) {
        int mini_exp = 777;
        int host_exp = 777;
        double mini_fraction = mini_test_frexp(decompose_values[i], &mini_exp);
        double host_fraction = frexp(decompose_values[i], &host_exp);
        double mini_integral;
        double host_integral;
        double mini_part = mini_test_modf(decompose_values[i], &mini_integral);
        double host_part = modf(decompose_values[i], &host_integral);

        if (!same_double(mini_fraction, host_fraction) || mini_exp != host_exp ||
            !same_double(mini_part, host_part) ||
            !same_double(mini_integral, host_integral)) {
            fprintf(stderr, "double decomposition mismatch at index %zu\n", i);
            return 5;
        }
    }

    for (i = 0; i < sizeof(fdecompose_values) / sizeof(fdecompose_values[0]); ++i) {
        int mini_exp = 777;
        int host_exp = 777;
        float mini_fraction = mini_test_frexpf(fdecompose_values[i], &mini_exp);
        float host_fraction = frexpf(fdecompose_values[i], &host_exp);
        float mini_integral;
        float host_integral;
        float mini_part = mini_test_modff(fdecompose_values[i], &mini_integral);
        float host_part = modff(fdecompose_values[i], &host_integral);

        if (!same_float(mini_fraction, host_fraction) || mini_exp != host_exp ||
            !same_float(mini_part, host_part) ||
            !same_float(mini_integral, host_integral)) {
            fprintf(stderr, "float decomposition mismatch at index %zu\n", i);
            return 6;
        }
    }

    for (i = 0; i < sizeof(scale_cases) / sizeof(scale_cases[0]); ++i) {
        if (!same_double(mini_test_scalbn(scale_cases[i].value, scale_cases[i].exponent),
                         scalbn(scale_cases[i].value, scale_cases[i].exponent)) ||
            !same_double(mini_test_ldexp(scale_cases[i].value, scale_cases[i].exponent),
                         ldexp(scale_cases[i].value, scale_cases[i].exponent))) {
            fprintf(stderr, "double scaling mismatch at index %zu\n", i);
            return 7;
        }
    }

    for (i = 0; i < sizeof(fscale_cases) / sizeof(fscale_cases[0]); ++i) {
        if (!same_float(mini_test_scalbnf(fscale_cases[i].value, fscale_cases[i].exponent),
                        scalbnf(fscale_cases[i].value, fscale_cases[i].exponent)) ||
            !same_float(mini_test_ldexpf(fscale_cases[i].value, fscale_cases[i].exponent),
                        ldexpf(fscale_cases[i].value, fscale_cases[i].exponent))) {
            fprintf(stderr, "float scaling mismatch at index %zu\n", i);
            return 8;
        }
    }

    *__mini_errno_location() = 101;
    if (dbits(mini_test_scalbn(1.0, -1074)) != 1ULL ||
        *__mini_errno_location() != 101) {
        return 9;
    }
    *__mini_errno_location() = 102;
    if (dbits(mini_test_scalbn(1.5, -1074)) != 2ULL ||
        *__mini_errno_location() != ERANGE) {
        return 10;
    }
    *__mini_errno_location() = 103;
    if (dbits(mini_test_ldexp(1.0, 1024)) != 0x7ff0000000000000ULL ||
        *__mini_errno_location() != ERANGE) {
        return 11;
    }

    for (i = 0; i < sizeof(roots) / sizeof(roots[0]); ++i) {
        if (!same_double(mini_test_sqrt(roots[i]), sqrt(roots[i]))) {
            fprintf(stderr, "double sqrt mismatch at index %zu\n", i);
            return 12;
        }
    }
    for (i = 0; i < sizeof(froots) / sizeof(froots[0]); ++i) {
        if (!same_float(mini_test_sqrtf(froots[i]), sqrtf(froots[i]))) {
            fprintf(stderr, "float sqrt mismatch at index %zu\n", i);
            return 13;
        }
    }

    *__mini_errno_location() = 91;
    if (!dnan(mini_test_sqrt(-1.0)) || *__mini_errno_location() != EDOM) {
        return 14;
    }
    *__mini_errno_location() = 92;
    if (!fnan(mini_test_sqrtf(-1.0f)) || *__mini_errno_location() != EDOM) {
        return 15;
    }

    puts("math differential passed");
    return 0;
}
