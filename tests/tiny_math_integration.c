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
    int quotient;

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

    errno = 83;
    if (!close_double(sin(0.5), 0.47942553860420300538, 3.0e-15) ||
        !close_double(cos(0.5), 0.87758256189037271612, 3.0e-15) ||
        !close_double(tan(0.5), 0.54630248984379051326, 5.0e-15) || errno != 83) {
        return 20;
    }
    if (!close_double(sin(1234.5), 0.14539565052293642557, 4.0e-14) ||
        !close_double(cos(1234.5), -0.98937359213242199729, 4.0e-14) ||
        !close_double(tan(1234.5), -0.14695727850342305132, 7.0e-14)) {
        return 21;
    }
    if (!close_float(sinf(0.5f), 0.47942555f, 2.0e-7f) ||
        !close_float(cosf(0.5f), 0.87758255f, 2.0e-7f) ||
        !close_float(tanf(0.5f), 0.5463025f, 3.0e-7f)) {
        return 22;
    }
    errno = 84;
    bad = sin(1048577.0);
    if (bad == bad || errno != EDOM) {
        return 23;
    }

    errno = 85;
    if (!close_double(atan(0.5), 0.46364760900080611621, 5.0e-16) ||
        !close_double(atan2(1.0, -1.0), 2.35619449019234492885, 7.0e-16) ||
        !close_double(asin(0.5), 0.52359877559829887308, 6.0e-16) ||
        !close_double(acos(0.5), 1.04719755119659774615, 7.0e-16) || errno != 85) {
        return 24;
    }
    if (!close_float(atanf(0.5f), 0.4636476f, 2.0e-7f) ||
        !close_float(atan2f(-1.0f, -1.0f), -2.3561945f, 3.0e-7f) ||
        !close_float(asinf(0.5f), 0.5235988f, 2.0e-7f) ||
        !close_float(acosf(0.5f), 1.0471976f, 3.0e-7f)) {
        return 25;
    }
    errno = 86;
    bad = asin(1.01);
    if (bad == bad || errno != EDOM) {
        return 26;
    }

    errno = 87;
    if (!close_double(sinh(1.0), 1.17520119364380145688, 5.0e-13) ||
        !close_double(cosh(1.0), 1.54308063481524377848, 5.0e-13) ||
        !close_double(tanh(1.0), 0.76159415595576488812, 5.0e-13) ||
        !close_double(asinh(1.0), 0.88137358701954302523, 5.0e-13) ||
        !close_double(acosh(2.0), 1.31695789692481670863, 5.0e-13) ||
        !close_double(atanh(0.5), 0.54930614433405484570, 5.0e-13) || errno != 87) {
        return 27;
    }
    if (!close_float(sinhf(1.0f), 1.1752012f, 4.0e-6f) ||
        !close_float(coshf(1.0f), 1.5430807f, 4.0e-6f) ||
        !close_float(tanhf(1.0f), 0.7615942f, 4.0e-6f) ||
        !close_float(asinhf(1.0f), 0.8813736f, 4.0e-6f) ||
        !close_float(acoshf(2.0f), 1.3169579f, 4.0e-6f) ||
        !close_float(atanhf(0.5f), 0.54930615f, 4.0e-6f)) {
        return 28;
    }
    errno = 88;
    bad = acosh(0.5);
    if (bad == bad || errno != EDOM) {
        return 29;
    }
    errno = 89;
    bad = atanh(1.0);
    if (!(bad > 1.0e308) || errno != ERANGE) {
        return 30;
    }

    errno = 90;
    if (fpclassify(scalbnf(1.0f, -149)) != FP_SUBNORMAL ||
        fpclassify(1.0) != FP_NORMAL || !isfinite(1.0f) || isfinite(bad) ||
        !signbit(-0.0) || !isless(1.0f, 2.0) || errno != 90) {
        return 31;
    }
    errno = 91;
    if (fmod(13.0, 4.0) != 1.0 || fmod(scalbn(1.0, 900), 3.0) != 1.0 ||
        remainder(6.0, 4.0) != -2.0 || errno != 91) {
        return 32;
    }
    quotient = 99;
    if (remquo(30.0, 4.0, &quotient) != -2.0 || quotient != 8) {
        return 33;
    }
    quotient = 99;
    if (remquof(-30.0f, -4.0f, &quotient) != 2.0f || quotient != 8 ||
        fmodf(13.0f, 4.0f) != 1.0f || remainderf(6.0f, 4.0f) != -2.0f) {
        return 34;
    }

    errno = 92;
    if (!close_double(erf(1.0), 0.84270079294971486934, 5.0e-13) ||
        !close_double(erfc(2.0), 0.00467773498104726584, 5.0e-15) || errno != 92) {
        return 35;
    }
    if (!close_float(erff(0.5f), 0.5204999f, 2.0e-7f) ||
        !close_float(erfcf(2.0f), 0.004677735f, 2.0e-8f)) {
        return 36;
    }
    errno = 93;
    if (erfc(30.0) != 0.0 || errno != ERANGE) {
        return 37;
    }
    errno = 94;
    if (erfc(-30.0) != 2.0 || errno != 94) {
        return 38;
    }

    errno = 95;
    if (!close_double(tgamma(0.5), 1.77245385090551602730, 5.0e-13) ||
        !close_double(tgamma(-0.5), -3.54490770181103205460, 5.0e-13) ||
        !close_double(lgamma(5.0), 3.17805383034794561965, 5.0e-13) ||
        errno != 95) {
        return 39;
    }
    if (!close_float(tgammaf(5.0f), 24.0f, 4.0e-5f) ||
        !close_float(lgammaf(-0.5f), 1.2655121f, 4.0e-5f)) {
        return 40;
    }
    errno = 96;
    bad = tgamma(-2.0);
    if (bad == bad || errno != EDOM) {
        return 41;
    }
    errno = 97;
    bad = tgamma(172.0);
    if (!(bad > 1.0e308) || errno != ERANGE) {
        return 42;
    }
    errno = 98;
    bad = lgamma(-2.0);
    if (!(bad > 1.0e308) || errno != ERANGE) {
        return 43;
    }

    if (puts("tiny-math-ok") == EOF) {
        return 44;
    }
    return 0;
}
