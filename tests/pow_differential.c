#include <errno.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>

extern double mini_test_pow(double x, double y);
extern float mini_test_powf(float x, float y);
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

static int dnan(double value)
{
    unsigned long long bits = dbits(value);

    return (bits & 0x7ff0000000000000ULL) == 0x7ff0000000000000ULL &&
           (bits & 0x000fffffffffffffULL) != 0ULL;
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
    static const struct {
        double x;
        double y;
    } cases[] = {
        {0.125, -2.5}, {0.5, -0.5}, {0.75, 3.25}, {1.25, 4.5},
        {2.0, -17.0}, {2.0, 17.0}, {3.0, 0.25}, {5.0, 1.25},
        {10.0, -3.5}, {123.5, 0.125}, {-2.0, 15.0}, {-2.0, 16.0},
        {-1.5, -7.0}, {-0.5, 9.0}
    };
    static const struct {
        float x;
        float y;
    } fcases[] = {
        {0.5f, -0.5f}, {0.75f, 3.25f}, {1.25f, 4.5f},
        {2.0f, -9.0f}, {2.0f, 9.0f}, {5.0f, 1.25f},
        {-2.0f, 7.0f}, {-2.0f, 8.0f}
    };
    size_t i;

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        double mini = mini_test_pow(cases[i].x, cases[i].y);
        double host = pow(cases[i].x, cases[i].y);

        if (!close_double(mini, host, 5.0e-12, 2.0e-14)) {
            fprintf(stderr,
                    "double pow mismatch at index %zu: %.17g %.17g\n",
                    i, mini, host);
            return 1;
        }
    }

    for (i = 0; i < sizeof(fcases) / sizeof(fcases[0]); ++i) {
        float mini = mini_test_powf(fcases[i].x, fcases[i].y);
        float host = powf(fcases[i].x, fcases[i].y);

        if (!close_float(mini, host, 8.0e-6f, 2.0e-6f)) {
            fprintf(stderr, "float pow mismatch at index %zu\n", i);
            return 2;
        }
    }

    *__mini_errno_location() = 91;
    if (mini_test_pow(1.25, 4.0) != 2.44140625 ||
        *__mini_errno_location() != 91) {
        return 3;
    }
    *__mini_errno_location() = 92;
    if (!dnan(mini_test_pow(-2.0, 0.5)) ||
        *__mini_errno_location() != EDOM) {
        return 4;
    }
    *__mini_errno_location() = 93;
    if ((dbits(mini_test_pow(10.0, 400.0)) & 0x7fffffffffffffffULL) !=
            0x7ff0000000000000ULL ||
        *__mini_errno_location() != ERANGE) {
        return 5;
    }
    *__mini_errno_location() = 94;
    if (dbits(mini_test_pow(10.0, -400.0)) != 0ULL ||
        *__mini_errno_location() != ERANGE) {
        return 6;
    }
    *__mini_errno_location() = 95;
    if ((fbits(mini_test_powf(10.0f, 100.0f)) & 0x7f800000U) !=
            0x7f800000U ||
        *__mini_errno_location() != ERANGE) {
        return 7;
    }

    puts("pow differential passed");
    return 0;
}
