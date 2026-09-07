#include <errno.h>
#include <math.h>
#include <stdio.h>

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

    if (puts("tiny-math-ok") == EOF) {
        return 13;
    }
    return 0;
}
