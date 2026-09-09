#include <fenv.h>
#include <math.h>
#include <stdio.h>

#define MINI_FE_INVALID 0x01
#define MINI_FE_DIVBYZERO 0x04
#define MINI_FE_OVERFLOW 0x08
#define MINI_FE_UNDERFLOW 0x10
#define MINI_FE_INEXACT 0x20

double mini_test_sin(double x);
float mini_test_sinf(float x);
double mini_test_cos(double x);
double mini_test_tan(double x);

static double from_bits(unsigned long long bits)
{
    union {
        double value;
        unsigned long long bits;
    } convert;

    convert.bits = bits;
    return convert.value;
}

static int only_flag(int flag)
{
    return fetestexcept(FE_ALL_EXCEPT) == flag;
}

int main(void)
{
    fenv_t saved;
    double value;

    if (FE_INVALID != MINI_FE_INVALID || FE_DIVBYZERO != MINI_FE_DIVBYZERO ||
        FE_OVERFLOW != MINI_FE_OVERFLOW || FE_UNDERFLOW != MINI_FE_UNDERFLOW ||
        FE_INEXACT != MINI_FE_INEXACT) {
        return 1;
    }
    if (fegetenv(&saved) != 0 || fesetenv(FE_DFL_ENV) != 0) {
        return 2;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 || mini_test_sin(-0.0) != -0.0 ||
        mini_test_cos(0.0) != 1.0 || mini_test_tan(0.0) != 0.0 ||
        fetestexcept(FE_ALL_EXCEPT) != 0) {
        return 3;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 4;
    }
    value = mini_test_sin(from_bits(0x7ff0000000000000ULL));
    if (!isnan(value) || !only_flag(FE_INVALID)) {
        return 5;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 6;
    }
    value = mini_test_cos(1048577.0);
    if (!isnan(value) || !only_flag(FE_INVALID)) {
        return 7;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 8;
    }
    value = mini_test_sin(0.5);
    if (!(value > 0.47 && value < 0.49) || !only_flag(FE_INEXACT)) {
        return 9;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 10;
    }
    value = mini_test_cos(0.5);
    if (!(value > 0.87 && value < 0.89) || !only_flag(FE_INEXACT)) {
        return 11;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 12;
    }
    value = mini_test_tan(0.5);
    if (!(value > 0.54 && value < 0.55) || !only_flag(FE_INEXACT)) {
        return 13;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 14;
    }
    value = (double)mini_test_sinf(0.5f);
    if (!(value > 0.47 && value < 0.49) || !only_flag(FE_INEXACT)) {
        return 15;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 || feraiseexcept(FE_UNDERFLOW) != 0 ||
        mini_test_sin(0.0) != 0.0 || mini_test_cos(0.0) != 1.0 ||
        mini_test_tan(0.0) != 0.0 ||
        fetestexcept(FE_ALL_EXCEPT) != FE_UNDERFLOW) {
        return 16;
    }

    if (fesetenv(&saved) != 0) {
        return 17;
    }
    if (puts("fenv-trig-interop-ok") == EOF) {
        return 18;
    }
    return 0;
}
