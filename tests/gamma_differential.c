#include <math.h>
#include <stdio.h>

extern double mini_test_tgamma(double x);
extern float mini_test_tgammaf(float x);
extern double mini_test_lgamma(double x);
extern float mini_test_lgammaf(float x);

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
    static const double values[] = {
        -20.75, -10.2, -5.25, -2.3, -1.5, -0.5, 0.1, 0.5,
        1.0, 1.5, 2.5, 5.0, 10.0, 20.0, 50.0, 100.0, 170.0
    };
    static const float fvalues[] = {
        -5.25f, -2.3f, -1.5f, -0.5f, 0.1f, 0.5f,
        1.5f, 2.5f, 5.0f, 10.0f, 30.0f
    };
    size_t i;

    for (i = 0; i < sizeof(values) / sizeof(values[0]); ++i) {
        double x = values[i];
        double actual_gamma = mini_test_tgamma(x);
        double expected_gamma = tgamma(x);
        double actual_lgamma = mini_test_lgamma(x);
        double expected_lgamma = lgamma(x);

        if (!close_double(actual_gamma, expected_gamma, 3.0e-12, 3.0e-15)) {
            fprintf(stderr, "tgamma mismatch at %.17g: %.17g vs %.17g\n",
                    x, actual_gamma, expected_gamma);
            return 1;
        }
        if (!close_double(actual_lgamma, expected_lgamma, 3.0e-12, 3.0e-14)) {
            fprintf(stderr, "lgamma mismatch at %.17g: %.17g vs %.17g\n",
                    x, actual_lgamma, expected_lgamma);
            return 2;
        }
    }

    for (i = 0; i < sizeof(fvalues) / sizeof(fvalues[0]); ++i) {
        float x = fvalues[i];
        float actual_gamma = mini_test_tgammaf(x);
        float expected_gamma = tgammaf(x);
        float actual_lgamma = mini_test_lgammaf(x);
        float expected_lgamma = lgammaf(x);

        if (!close_float(actual_gamma, expected_gamma, 2.0e-5f, 4.0e-6f) ||
            !close_float(actual_lgamma, expected_lgamma, 2.0e-5f, 4.0e-6f)) {
            fprintf(stderr, "float gamma mismatch at %.9g\n", (double)x);
            return 3;
        }
    }

    puts("gamma differential passed");
    return 0;
}
