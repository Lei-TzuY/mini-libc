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

    for (i = 0; i < sizeof(roots) / sizeof(roots[0]); ++i) {
        if (!same_double(mini_test_sqrt(roots[i]), sqrt(roots[i]))) {
            fprintf(stderr, "double sqrt mismatch at index %zu\n", i);
            return 5;
        }
    }
    for (i = 0; i < sizeof(froots) / sizeof(froots[0]); ++i) {
        if (!same_float(mini_test_sqrtf(froots[i]), sqrtf(froots[i]))) {
            fprintf(stderr, "float sqrt mismatch at index %zu\n", i);
            return 6;
        }
    }

    *__mini_errno_location() = 91;
    if (!dnan(mini_test_sqrt(-1.0)) || *__mini_errno_location() != EDOM) {
        return 7;
    }
    *__mini_errno_location() = 92;
    if (!fnan(mini_test_sqrtf(-1.0f)) || *__mini_errno_location() != EDOM) {
        return 8;
    }

    puts("math differential passed");
    return 0;
}
