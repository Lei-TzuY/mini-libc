#include <errno.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>

extern double mini_test_atan(double x);
extern float mini_test_atanf(float x);
extern double mini_test_atan2(double y, double x);
extern float mini_test_atan2f(float y, float x);
extern double mini_test_asin(double x);
extern float mini_test_asinf(float x);
extern double mini_test_acos(double x);
extern float mini_test_acosf(float x);
extern int *__mini_errno_location(void);

static double absolute(double value)
{
    return value < 0.0 ? -value : value;
}

static int close_double(double actual, double expected,
                        double relative_tolerance, double absolute_tolerance)
{
    double difference = absolute(actual - expected);
    double scale = absolute(expected);

    return difference <= absolute_tolerance + relative_tolerance * scale;
}

static int close_float(float actual, float expected,
                       float relative_tolerance, float absolute_tolerance)
{
    float difference = actual - expected;
    float scale = expected;

    if (difference < 0.0f) {
        difference = -difference;
    }
    if (scale < 0.0f) {
        scale = -scale;
    }
    return difference <= absolute_tolerance + relative_tolerance * scale;
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

static double dfrom(unsigned long long bits)
{
    union {
        double value;
        unsigned long long bits;
    } convert;

    convert.bits = bits;
    return convert.value;
}

int main(void)
{
    static const double atan_cases[] = {
        -1.0e300, -1.0e20, -1000000.0, -1000.0, -10.0, -3.0,
        -1.0, -0.5, -0.125, -1.0e-12, 0.0, 1.0e-12, 0.125,
        0.5, 1.0, 3.0, 10.0, 1000.0, 1000000.0, 1.0e20, 1.0e300
    };
    static const double atan2_cases[][2] = {
        {1.0, 1.0}, {1.0, -1.0}, {-1.0, -1.0}, {-1.0, 1.0},
        {3.0, 4.0}, {4.0, 3.0}, {-3.0, 4.0}, {3.0, -4.0},
        {1.0e300, 1.0e-300}, {1.0e-300, 1.0e300},
        {-1.0e300, -1.0e-300}, {1.0e-300, -1.0e300}
    };
    static const double unit_cases[] = {
        -1.0, -0.999999999999, -0.9, -0.5, -0.125, 0.0,
        0.125, 0.5, 0.9, 0.999999999999, 1.0
    };
    static const float float_cases[] = {
        -1000.0f, -3.0f, -1.0f, -0.5f, -0.125f,
        0.0f, 0.125f, 0.5f, 1.0f, 3.0f, 1000.0f
    };
    size_t i;

    for (i = 0; i < sizeof(atan_cases) / sizeof(atan_cases[0]); ++i) {
        double x = atan_cases[i];
        double actual = mini_test_atan(x);
        double expected = atan(x);

        if (!close_double(actual, expected, 3.0e-15, 4.0e-16)) {
            fprintf(stderr, "atan mismatch at %zu: %.17g %.17g\n",
                    i, actual, expected);
            return 1;
        }
    }

    for (i = 0; i < sizeof(atan2_cases) / sizeof(atan2_cases[0]); ++i) {
        double y = atan2_cases[i][0];
        double x = atan2_cases[i][1];
        double actual = mini_test_atan2(y, x);
        double expected = atan2(y, x);

        if (!close_double(actual, expected, 3.0e-15, 6.0e-16)) {
            fprintf(stderr, "atan2 mismatch at %zu: %.17g %.17g\n",
                    i, actual, expected);
            return 2;
        }
    }

    for (i = 0; i < sizeof(unit_cases) / sizeof(unit_cases[0]); ++i) {
        double x = unit_cases[i];
        double actual_asin = mini_test_asin(x);
        double actual_acos = mini_test_acos(x);
        double expected_asin = asin(x);
        double expected_acos = acos(x);

        if (!close_double(actual_asin, expected_asin, 8.0e-15, 8.0e-16) ||
            !close_double(actual_acos, expected_acos, 8.0e-15, 9.0e-16)) {
            fprintf(stderr, "asin/acos mismatch at %zu\n", i);
            return 3;
        }
    }

    for (i = 0; i < sizeof(float_cases) / sizeof(float_cases[0]); ++i) {
        float x = float_cases[i];

        if (!close_float(mini_test_atanf(x), atanf(x), 5.0e-6f, 2.0e-7f)) {
            fprintf(stderr, "atanf mismatch at %zu\n", i);
            return 4;
        }
        if (x >= -1.0f && x <= 1.0f &&
            (!close_float(mini_test_asinf(x), asinf(x), 5.0e-6f, 3.0e-7f) ||
             !close_float(mini_test_acosf(x), acosf(x), 5.0e-6f, 3.0e-7f))) {
            fprintf(stderr, "asinf/acosf mismatch at %zu\n", i);
            return 5;
        }
    }

    if (!close_float(mini_test_atan2f(1.0f, -1.0f), atan2f(1.0f, -1.0f),
                     5.0e-6f, 3.0e-7f) ||
        !close_float(mini_test_atan2f(-3.0f, 4.0f), atan2f(-3.0f, 4.0f),
                     5.0e-6f, 3.0e-7f)) {
        return 6;
    }

    *__mini_errno_location() = 91;
    if (!close_double(mini_test_atan(0.5), atan(0.5), 1.0e-14, 4.0e-16) ||
        *__mini_errno_location() != 91 ||
        !close_double(mini_test_atan2(3.0, 4.0), atan2(3.0, 4.0),
                      1.0e-14, 5.0e-16) ||
        *__mini_errno_location() != 91 ||
        !close_double(mini_test_asin(0.5), asin(0.5), 1.0e-14, 5.0e-16) ||
        *__mini_errno_location() != 91 ||
        !close_double(mini_test_acos(0.5), acos(0.5), 1.0e-14, 6.0e-16) ||
        *__mini_errno_location() != 91) {
        return 7;
    }

    *__mini_errno_location() = 92;
    if (mini_test_asin(1.01) == mini_test_asin(1.01) ||
        *__mini_errno_location() != EDOM) {
        return 8;
    }
    *__mini_errno_location() = 93;
    if (mini_test_acos(-1.01) == mini_test_acos(-1.01) ||
        *__mini_errno_location() != EDOM) {
        return 9;
    }

    *__mini_errno_location() = 94;
    if (dbits(mini_test_atan(dfrom(0x7ff8000000001234ULL))) !=
            0x7ff8000000001234ULL ||
        *__mini_errno_location() != 94) {
        return 10;
    }

    puts("inverse trig differential passed");
    return 0;
}
