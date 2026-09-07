#include <fenv.h>
#include <math.h>
#include <stdio.h>

#define MINI_FE_INVALID 0x01
#define MINI_FE_DIVBYZERO 0x04
#define MINI_FE_OVERFLOW 0x08
#define MINI_FE_UNDERFLOW 0x10
#define MINI_FE_INEXACT 0x20

double mini_test_pow(double x, double y);
float mini_test_powf(float x, float y);

static int has_flags(int flags)
{
    return (fetestexcept(flags) & flags) == flags;
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

    if (feclearexcept(FE_ALL_EXCEPT) != 0 || mini_test_pow(2.0, 3.0) != 8.0 ||
        fetestexcept(FE_ALL_EXCEPT) != 0) {
        return 3;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 4;
    }
    value = mini_test_pow(2.0, 0.5);
    if (!(value > 1.41 && value < 1.42) || !has_flags(FE_INEXACT)) {
        return 5;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 6;
    }
    value = mini_test_pow(-2.0, 0.5);
    if (!isnan(value) || !has_flags(FE_INVALID)) {
        return 7;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 8;
    }
    value = mini_test_pow(0.0, -3.0);
    if (!isinf(value) || signbit(value) || !has_flags(FE_DIVBYZERO)) {
        return 9;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 10;
    }
    value = mini_test_pow(2.0, 1024.0);
    if (!isinf(value) || signbit(value) ||
        !has_flags(FE_OVERFLOW | FE_INEXACT)) {
        return 11;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        mini_test_pow(2.0, -1075.0) != 0.0 ||
        !has_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 12;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        !isinf(mini_test_powf(2.0f, 128.0f)) ||
        !has_flags(FE_OVERFLOW | FE_INEXACT)) {
        return 13;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        mini_test_powf(2.0f, -150.0f) != 0.0f ||
        !has_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 14;
    }

    if (fesetenv(&saved) != 0) {
        return 15;
    }
    if (puts("fenv-pow-interop-ok") == EOF) {
        return 16;
    }
    return 0;
}
