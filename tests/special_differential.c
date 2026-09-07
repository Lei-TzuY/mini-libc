#include <math.h>
#include <stdio.h>

extern double mini_test_erf(double x);
extern float mini_test_erff(float x);
extern double mini_test_erfc(double x);
extern float mini_test_erfcf(float x);

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
        -20.0, -10.0, -6.0, -4.0, -2.0, -1.5, -1.0, -0.75,
        -0.5, -0.1, -1.0e-8, 0.0, 1.0e-8, 0.1, 0.5, 0.75,
        1.0, 1.5, 2.0, 4.0, 6.0, 8.0, 10.0, 20.0, 26.0
    };
    static const float fvalues[] = {
        -6.0f, -4.0f, -2.0f, -1.0f, -0.5f, -0.1f,
        0.0f, 0.1f, 0.5f, 1.0f, 2.0f, 4.0f, 6.0f
    };
    size_t i;

    for (i = 0; i < sizeof(values) / sizeof(values[0]); ++i) {
        double x = values[i];
        double actual_erf = mini_test_erf(x);
        double expected_erf = erf(x);
        double actual_erfc = mini_test_erfc(x);
        double expected_erfc = erfc(x);

        if (!close_double(actual_erf, expected_erf, 8.0e-13, 3.0e-15)) {
            fprintf(stderr, "erf mismatch at %.17g: %.17g vs %.17g\n",
                    x, actual_erf, expected_erf);
            return 1;
        }
        if (!close_double(actual_erfc, expected_erfc, 8.0e-13, 3.0e-15)) {
            fprintf(stderr, "erfc mismatch at %.17g: %.17g vs %.17g\n",
                    x, actual_erfc, expected_erfc);
            return 2;
        }
    }

    for (i = 0; i < sizeof(fvalues) / sizeof(fvalues[0]); ++i) {
        float x = fvalues[i];
        float actual_erf = mini_test_erff(x);
        float expected_erf = erff(x);
        float actual_erfc = mini_test_erfcf(x);
        float expected_erfc = erfcf(x);

        if (!close_float(actual_erf, expected_erf, 6.0e-6f, 3.0e-7f) ||
            !close_float(actual_erfc, expected_erfc, 6.0e-6f, 3.0e-7f)) {
            fprintf(stderr, "float error-function mismatch at %.9g\n", (double)x);
            return 3;
        }
    }

    puts("special differential passed");
    return 0;
}
