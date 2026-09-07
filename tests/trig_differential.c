#include <errno.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>

extern double mini_test_sin(double x);
extern float mini_test_sinf(float x);
extern double mini_test_cos(double x);
extern float mini_test_cosf(float x);
extern double mini_test_tan(double x);
extern float mini_test_tanf(float x);
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

static double dfrom(unsigned long long bits)
{
    union {
        double value;
        unsigned long long bits;
    } convert;

    convert.bits = bits;
    return convert.value;
}

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

int main(void)
{
    static const double cases[] = {
        -1000000.0, -12345.25, -1234.5, -100.125, -10.0,
        -3.0, -1.25, -0.75, -0.125, 0.0, 0.125, 0.75,
        1.25, 3.0, 10.0, 100.125, 1234.5, 12345.25, 1000000.0
    };
    static const float fcases[] = {
        -1000.25f, -10.0f, -1.25f, -0.25f,
        0.0f, 0.25f, 1.25f, 10.0f, 1000.25f
    };
    size_t i;

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        double x = cases[i];
        double ms = mini_test_sin(x);
        double mc = mini_test_cos(x);
        double mt = mini_test_tan(x);
        double hs = sin(x);
        double hc = cos(x);
        double ht = tan(x);

        if (!close_double(ms, hs, 2.0e-13, 3.0e-15)) {
            fprintf(stderr, "double sin mismatch at index %zu: %.17g %.17g\n",
                    i, ms, hs);
            return 1;
        }
        if (!close_double(mc, hc, 2.0e-13, 3.0e-15)) {
            fprintf(stderr, "double cos mismatch at index %zu: %.17g %.17g\n",
                    i, mc, hc);
            return 2;
        }
        if (!close_double(mt, ht, 8.0e-12, 8.0e-14)) {
            fprintf(stderr, "double tan mismatch at index %zu: %.17g %.17g\n",
                    i, mt, ht);
            return 3;
        }
    }

    for (i = 0; i < sizeof(fcases) / sizeof(fcases[0]); ++i) {
        float x = fcases[i];

        if (!close_float(mini_test_sinf(x), sinf(x), 5.0e-6f, 3.0e-7f) ||
            !close_float(mini_test_cosf(x), cosf(x), 5.0e-6f, 3.0e-7f) ||
            !close_float(mini_test_tanf(x), tanf(x), 8.0e-6f, 8.0e-7f)) {
            fprintf(stderr, "float trig mismatch at index %zu\n", i);
            return 4;
        }
    }

    *__mini_errno_location() = 81;
    if (!close_double(mini_test_sin(0.5), sin(0.5), 1.0e-14, 2.0e-15) ||
        *__mini_errno_location() != 81) {
        return 5;
    }

    *__mini_errno_location() = 82;
    if (mini_test_sin(dfrom(0x7ff0000000000000ULL)) ==
            mini_test_sin(dfrom(0x7ff0000000000000ULL)) ||
        *__mini_errno_location() != EDOM) {
        return 6;
    }

    *__mini_errno_location() = 83;
    if (mini_test_cos(1048577.0) == mini_test_cos(1048577.0) ||
        *__mini_errno_location() != EDOM) {
        return 7;
    }

    *__mini_errno_location() = 84;
    if (dbits(mini_test_sin(dfrom(0x7ff8000000001234ULL))) !=
            0x7ff8000000001234ULL ||
        *__mini_errno_location() != 84) {
        return 8;
    }

    puts("trig differential passed");
    return 0;
}
