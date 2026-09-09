#include <fenv.h>
#include <math.h>
#include <stdio.h>

double mini_test_fmod(double x, double y);
float mini_test_fmodf(float x, float y);
double mini_test_remainder(double x, double y);
float mini_test_remainderf(float x, float y);
double mini_test_remquo(double x, double y, int *quo);
float mini_test_remquof(float x, float y, int *quo);

static double from_bits(unsigned long long bits)
{
    union {
        double value;
        unsigned long long bits;
    } convert;

    convert.bits = bits;
    return convert.value;
}

static int only_flags(int flags)
{
    return fetestexcept(FE_ALL_EXCEPT) == flags;
}

int main(void)
{
    fenv_t saved;
    double value;
    float fvalue;
    int quotient;
    double inf = from_bits(0x7ff0000000000000ULL);
    double nan_value = from_bits(0x7ff8123456789abcULL);

    if (fegetenv(&saved) != 0 || fesetenv(FE_DFL_ENV) != 0 ||
        feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 1;
    }

    quotient = 99;
    if (mini_test_fmod(13.0, 4.0) != 1.0 ||
        mini_test_remainder(6.0, 4.0) != -2.0 ||
        mini_test_remquo(30.0, 4.0, &quotient) != -2.0 || quotient != 8 ||
        !only_flags(0)) {
        return 2;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        feraiseexcept(FE_DIVBYZERO) != 0) {
        return 3;
    }
    value = mini_test_fmod(5.3, 2.0);
    if (!(value > 1.29 && value < 1.31) || !only_flags(FE_DIVBYZERO)) {
        return 4;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 5;
    }
    value = mini_test_fmod(1.0, 0.0);
    if (!isnan(value) || !only_flags(FE_INVALID)) {
        return 6;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 7;
    }
    value = mini_test_remainder(inf, 2.0);
    if (!isnan(value) || !only_flags(FE_INVALID)) {
        return 8;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        feraiseexcept(FE_OVERFLOW) != 0) {
        return 9;
    }
    quotient = 99;
    value = mini_test_remquo(1.0, 0.0, &quotient);
    if (!isnan(value) || quotient != 0 ||
        !only_flags(FE_OVERFLOW | FE_INVALID)) {
        return 10;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 11;
    }
    fvalue = mini_test_remainderf(1.0f, 0.0f);
    if (!isnan(fvalue) || !only_flags(FE_INVALID)) {
        return 12;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 13;
    }
    quotient = 99;
    fvalue = mini_test_remquof(-30.0f, -4.0f, &quotient);
    if (fvalue != 2.0f || quotient != 8 || !only_flags(0)) {
        return 14;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        feraiseexcept(FE_UNDERFLOW) != 0) {
        return 15;
    }
    value = mini_test_fmod(nan_value, 2.0);
    if (!isnan(value) || mini_test_fmod(3.0, inf) != 3.0 ||
        !only_flags(FE_UNDERFLOW)) {
        return 16;
    }

    if (fesetenv(&saved) != 0) {
        return 17;
    }
    if (puts("fenv-remainder-interop-ok") == EOF) {
        return 18;
    }
    return 0;
}
