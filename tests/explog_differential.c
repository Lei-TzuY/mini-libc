#include <errno.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>

extern double mini_test_exp(double x);
extern float mini_test_expf(float x);
extern double mini_test_log(double x);
extern float mini_test_logf(float x);
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

static int fnan(float value)
{
    unsigned int bits = fbits(value);

    return (bits & 0x7f800000U) == 0x7f800000U &&
           (bits & 0x007fffffU) != 0U;
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

static int close_float(float actual, float expected, float relative_tolerance,
                       float absolute_tolerance)
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
    static const double exp_inputs[] = {
        -700.0, -100.0, -10.0, -1.0, -0.5, -0.125,
        0.0, 0.125, 0.5, 1.0, 10.0, 100.0, 700.0
    };
    static const double log_inputs[] = {
        0x0.0000000000001p-1022, 0x1p-1022, 1.0e-200,
        0.125, 0.5, 0.75, 1.0, 1.5, 2.0, 10.0, 1.0e100
    };
    static const float fexp_inputs[] = {
        -80.0f, -10.0f, -1.0f, -0.25f, 0.0f, 0.25f, 1.0f, 10.0f, 80.0f
    };
    static const float flog_inputs[] = {
        0x1p-126f, 0.125f, 0.5f, 0.75f, 1.0f, 1.5f, 2.0f, 10.0f, 1.0e30f
    };
    double qnan = dfrom(0x7ff8000000001234ULL);
    double inf = dfrom(0x7ff0000000000000ULL);
    size_t i;

    for (i = 0; i < sizeof(exp_inputs) / sizeof(exp_inputs[0]); ++i) {
        double mini = mini_test_exp(exp_inputs[i]);
        double host = exp(exp_inputs[i]);

        if (!close_double(mini, host, 2.0e-13, 1.0e-300)) {
            fprintf(stderr, "double exp mismatch at index %zu: %.17g %.17g\n",
                    i, mini, host);
            return 1;
        }
    }

    for (i = 0; i < sizeof(log_inputs) / sizeof(log_inputs[0]); ++i) {
        double mini = mini_test_log(log_inputs[i]);
        double host = log(log_inputs[i]);

        if (!close_double(mini, host, 2.0e-13, 2.0e-12)) {
            fprintf(stderr, "double log mismatch at index %zu: %.17g %.17g\n",
                    i, mini, host);
            return 2;
        }
    }

    for (i = 0; i < sizeof(fexp_inputs) / sizeof(fexp_inputs[0]); ++i) {
        float mini = mini_test_expf(fexp_inputs[i]);
        float host = expf(fexp_inputs[i]);

        if (!close_float(mini, host, 4.0e-6f, 1.0e-37f)) {
            fprintf(stderr, "float exp mismatch at index %zu\n", i);
            return 3;
        }
    }

    for (i = 0; i < sizeof(flog_inputs) / sizeof(flog_inputs[0]); ++i) {
        float mini = mini_test_logf(flog_inputs[i]);
        float host = logf(flog_inputs[i]);

        if (!close_float(mini, host, 4.0e-6f, 4.0e-6f)) {
            fprintf(stderr, "float log mismatch at index %zu\n", i);
            return 4;
        }
    }

    if (dbits(mini_test_exp(inf)) != dbits(inf) ||
        dbits(mini_test_exp(dfrom(0xfff0000000000000ULL))) != 0ULL ||
        dbits(mini_test_exp(qnan)) != dbits(qnan) ||
        dbits(mini_test_log(inf)) != dbits(inf) ||
        dbits(mini_test_log(qnan)) != dbits(qnan)) {
        return 5;
    }

    *__mini_errno_location() = 91;
    if (!close_double(mini_test_exp(0.25), exp(0.25), 2.0e-13, 0.0) ||
        *__mini_errno_location() != 91) {
        return 6;
    }
    *__mini_errno_location() = 92;
    if (!close_double(mini_test_log(4.0), log(4.0), 2.0e-13, 2.0e-15) ||
        *__mini_errno_location() != 92) {
        return 7;
    }

    *__mini_errno_location() = 93;
    if (dbits(mini_test_exp(1000.0)) != dbits(inf) ||
        *__mini_errno_location() != ERANGE) {
        return 8;
    }
    *__mini_errno_location() = 94;
    if (dbits(mini_test_exp(-1000.0)) != 0ULL ||
        *__mini_errno_location() != ERANGE) {
        return 9;
    }
    *__mini_errno_location() = 95;
    if (dbits(mini_test_exp(-744.0)) == 0ULL ||
        *__mini_errno_location() != ERANGE) {
        return 10;
    }
    *__mini_errno_location() = 96;
    if (dbits(mini_test_log(0.0)) != 0xfff0000000000000ULL ||
        *__mini_errno_location() != ERANGE) {
        return 11;
    }
    *__mini_errno_location() = 97;
    if (!dnan(mini_test_log(-1.0)) || *__mini_errno_location() != EDOM) {
        return 12;
    }
    *__mini_errno_location() = 98;
    if (!fnan(mini_test_logf(-1.0f)) || *__mini_errno_location() != EDOM) {
        return 13;
    }

    puts("exp/log differential passed");
    return 0;
}
