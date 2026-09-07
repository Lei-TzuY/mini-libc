#include <math.h>
#include <stdio.h>

extern double mini_test_sinh(double x);
extern float mini_test_sinhf(float x);
extern double mini_test_cosh(double x);
extern float mini_test_coshf(float x);
extern double mini_test_tanh(double x);
extern float mini_test_tanhf(float x);
extern double mini_test_asinh(double x);
extern float mini_test_asinhf(float x);
extern double mini_test_acosh(double x);
extern float mini_test_acoshf(float x);
extern double mini_test_atanh(double x);
extern float mini_test_atanhf(float x);

static int close_double(double actual, double expected, double rel, double abs)
{
    double difference = actual - expected;
    double magnitude = expected;

    if (difference < 0.0) {
        difference = -difference;
    }
    if (magnitude < 0.0) {
        magnitude = -magnitude;
    }
    return difference <= abs || difference <= rel * magnitude;
}

static int close_float(float actual, float expected, float rel, float abs)
{
    float difference = actual - expected;
    float magnitude = expected;

    if (difference < 0.0f) {
        difference = -difference;
    }
    if (magnitude < 0.0f) {
        magnitude = -magnitude;
    }
    return difference <= abs || difference <= rel * magnitude;
}

int main(void)
{
    static const double forward[] = {
        -20.0, -5.0, -1.0, -0.5, -0.25, -1.0e-8,
         0.0, 1.0e-8, 0.25, 0.5, 1.0, 5.0, 20.0, 700.0
    };
    static const double asinh_values[] = {
        -1.0e150, -100.0, -10.0, -1.0, -0.25, -1.0e-8,
         0.0, 1.0e-8, 0.25, 1.0, 10.0, 100.0, 1.0e150
    };
    static const double acosh_values[] = {
        1.0, 1.000000000001, 1.01, 1.5, 2.0, 10.0, 1.0e100, 1.0e150
    };
    static const double atanh_values[] = {
        -0.999999, -0.9, -0.5, -0.25, -1.0e-8, 0.0,
         1.0e-8, 0.25, 0.5, 0.9, 0.999999
    };
    static const float forward_f[] = {
        -5.0f, -1.0f, -0.25f, 0.0f, 0.25f, 1.0f, 5.0f
    };
    static const float inverse_f[] = {
        -0.9f, -0.5f, -0.25f, 0.25f, 0.5f, 0.9f
    };
    unsigned int i;

    for (i = 0; i < sizeof(forward) / sizeof(forward[0]); ++i) {
        double x = forward[i];

        if (!close_double(mini_test_sinh(x), sinh(x), 8.0e-13, 8.0e-14) ||
            !close_double(mini_test_cosh(x), cosh(x), 8.0e-13, 8.0e-14) ||
            !close_double(mini_test_tanh(x), tanh(x), 8.0e-13, 8.0e-14)) {
            fprintf(stderr, "forward hyperbolic mismatch at %.17g\n", x);
            return 1;
        }
    }

    for (i = 0; i < sizeof(asinh_values) / sizeof(asinh_values[0]); ++i) {
        double x = asinh_values[i];

        if (!close_double(mini_test_asinh(x), asinh(x), 2.0e-12, 2.0e-13)) {
            fprintf(stderr, "asinh mismatch at %.17g\n", x);
            return 2;
        }
    }

    for (i = 0; i < sizeof(acosh_values) / sizeof(acosh_values[0]); ++i) {
        double x = acosh_values[i];

        if (!close_double(mini_test_acosh(x), acosh(x), 3.0e-12, 3.0e-13)) {
            fprintf(stderr, "acosh mismatch at %.17g\n", x);
            return 3;
        }
    }

    for (i = 0; i < sizeof(atanh_values) / sizeof(atanh_values[0]); ++i) {
        double x = atanh_values[i];

        if (!close_double(mini_test_atanh(x), atanh(x), 2.0e-12, 2.0e-13)) {
            fprintf(stderr, "atanh mismatch at %.17g\n", x);
            return 4;
        }
    }

    for (i = 0; i < sizeof(forward_f) / sizeof(forward_f[0]); ++i) {
        float x = forward_f[i];

        if (!close_float(mini_test_sinhf(x), sinhf(x), 5.0e-6f, 3.0e-6f) ||
            !close_float(mini_test_coshf(x), coshf(x), 5.0e-6f, 3.0e-6f) ||
            !close_float(mini_test_tanhf(x), tanhf(x), 5.0e-6f, 3.0e-6f)) {
            fprintf(stderr, "float forward hyperbolic mismatch at %.9g\n", (double)x);
            return 5;
        }
    }

    for (i = 0; i < sizeof(inverse_f) / sizeof(inverse_f[0]); ++i) {
        float x = inverse_f[i];
        float acosh_input = x + 2.0f;

        if (!close_float(mini_test_asinhf(x), asinhf(x), 6.0e-6f, 3.0e-6f) ||
            !close_float(mini_test_atanhf(x), atanhf(x), 6.0e-6f, 3.0e-6f) ||
            !close_float(mini_test_acoshf(acosh_input), acoshf(acosh_input),
                         6.0e-6f, 3.0e-6f)) {
            fprintf(stderr, "float inverse hyperbolic mismatch at %.9g\n", (double)x);
            return 6;
        }
    }

    puts("hyperbolic differential passed");
    return 0;
}
