#include <errno.h>
#include <math.h>
#include <stdio.h>

static int close_double(double actual, double expected, double tolerance)
{
    double difference = actual - expected;

    if (difference < 0.0) {
        difference = -difference;
    }
    return difference <= tolerance;
}

static int close_float(float actual, float expected, float tolerance)
{
    float difference = actual - expected;

    if (difference < 0.0f) {
        difference = -difference;
    }
    return difference <= tolerance;
}

int main(void)
{
    double bad;
    double fraction;
    double integral;
    float fintegral;
    int exponent;

    if (fabs(-3.5) != 3.5 || fabsf(-2.25f) != 2.25f) {
        return 1;
    }
    if (copysign(2.0, -1.0) != -2.0 ||
        copysignf(2.0f, -1.0f) != -2.0f) {
        return 2;
    }
    if (fmin(-0.0, 0.0) != 0.0 || fmax(-0.0, 0.0) != 0.0) {
        return 3;
    }
    if (trunc(-1.75) != -1.0 || floor(-1.25) != -2.0 ||
        ceil(1.25) != 2.0 || round(2.5) != 3.0 || round(-2.5) != -3.0) {
        return 4;
    }

    exponent = 99;
    fraction = frexp(8.0, &exponent);
    if (fraction != 0.5 || exponent != 4) {
        return 5;
    }
    if (ldexp(0.75, 4) != 12.0 || scalbnf(0.75f, 4) != 12.0f) {
        return 6;
    }
    errno = 61;
    fraction = scalbn(1.0, -1074);
    if (fraction == 0.0 || errno != 61) {
        return 7;
    }
    exponent = 99;
    if (frexp(fraction, &exponent) != 0.5 || exponent != -1073) {
        return 8;
    }
    errno = 62;
    if (scalbn(1.0, -1075) != 0.0 || errno != ERANGE) {
        return 9;
    }
    if (modf(-3.25, &integral) != -0.25 || integral != -3.0 ||
        modff(-3.25f, &fintegral) != -0.25f || fintegral != -3.0f) {
        return 10;
    }

    if (sqrt(81.0) != 9.0 || sqrtf(16.0f) != 4.0f) {
        return 11;
    }

    errno = 77;
    bad = sqrt(-1.0);
    if (bad == bad || errno != EDOM) {
        return 12;
    }

    errno = 78;
    fraction = exp(1.0);
    if (!close_double(fraction, 2.71828182845904523536, 3.0e-15) || errno != 78 ||
        !close_double(log(2.0), 0.69314718055994530942, 2.0e-15) || errno != 78) {
        return 13;
    }
    if (!close_float(expf(1.0f), 2.7182817f, 2.0e-6f) ||
        !close_float(logf(2.0f), 0.6931472f, 2.0e-6f)) {
        return 14;
    }

    errno = 79;
    bad = exp(1000.0);
    if (!(bad > 1.0e308) || errno != ERANGE) {
        return 15;
    }
    errno = 80;
    bad = log(-1.0);
    if (bad == bad || errno != EDOM) {
        return 16;
    }

    errno = 81;
    if (pow(-2.0, 9.0) != -512.0 || pow(2.0, -10.0) != 0.0009765625 ||
        errno != 81) {
        return 17;
    }
    if (!close_double(pow(9.0, 0.5), 3.0, 2.0e-12) ||
        !close_float(powf(5.0f, 1.25f), 7.476744f, 5.0e-6f)) {
        return 18;
    }
    errno = 82;
    bad = pow(-2.0, 0.5);
    if (bad == bad || errno != EDOM) {
        return 19;
    }

    if (puts("tiny-math-ok") == EOF) {
        return 20;
    }
    return 0;
}
