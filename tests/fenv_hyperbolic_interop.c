#include <fenv.h>
#include <math.h>
#include <stdio.h>

#define MINI_FE_INVALID 0x01
#define MINI_FE_DIVBYZERO 0x04
#define MINI_FE_OVERFLOW 0x08
#define MINI_FE_UNDERFLOW 0x10
#define MINI_FE_INEXACT 0x20

double mini_test_sinh(double x);
float mini_test_sinhf(float x);
double mini_test_cosh(double x);
float mini_test_coshf(float x);
double mini_test_tanh(double x);
float mini_test_tanhf(float x);

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

    if (feclearexcept(FE_ALL_EXCEPT) != 0 || mini_test_sinh(0.0) != 0.0 ||
        mini_test_cosh(0.0) != 1.0 || mini_test_tanh(0.0) != 0.0 ||
        fetestexcept(FE_ALL_EXCEPT) != 0) {
        return 3;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 4;
    }
    value = mini_test_sinh(1.0);
    if (!(value > 1.17 && value < 1.18) || !has_flags(FE_INEXACT)) {
        return 5;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 6;
    }
    value = mini_test_cosh(1.0);
    if (!(value > 1.54 && value < 1.55) || !has_flags(FE_INEXACT)) {
        return 7;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 8;
    }
    value = mini_test_tanh(1.0);
    if (!(value > 0.76 && value < 0.77) || !has_flags(FE_INEXACT)) {
        return 9;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 10;
    }
    value = mini_test_sinh(711.0);
    if (!isinf(value) || signbit(value) ||
        !has_flags(FE_OVERFLOW | FE_INEXACT)) {
        return 11;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 12;
    }
    value = mini_test_cosh(711.0);
    if (!isinf(value) || signbit(value) ||
        !has_flags(FE_OVERFLOW | FE_INEXACT)) {
        return 13;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 14;
    }
    value = (double)mini_test_sinhf(90.0f);
    if (!isinf(value) || !has_flags(FE_OVERFLOW | FE_INEXACT)) {
        return 15;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 16;
    }
    value = (double)mini_test_coshf(90.0f);
    if (!isinf(value) || !has_flags(FE_OVERFLOW | FE_INEXACT)) {
        return 17;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 || mini_test_tanh(30.0) != 1.0 ||
        !has_flags(FE_INEXACT) ||
        fetestexcept(FE_OVERFLOW | FE_UNDERFLOW | FE_DIVBYZERO | FE_INVALID) != 0) {
        return 18;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 || mini_test_tanhf(-30.0f) != -1.0f ||
        !has_flags(FE_INEXACT) ||
        fetestexcept(FE_OVERFLOW | FE_UNDERFLOW | FE_DIVBYZERO | FE_INVALID) != 0) {
        return 19;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 || feraiseexcept(FE_UNDERFLOW) != 0 ||
        mini_test_sinh(0.0) != 0.0 || mini_test_cosh(0.0) != 1.0 ||
        mini_test_tanh(0.0) != 0.0 || fetestexcept(FE_ALL_EXCEPT) != FE_UNDERFLOW) {
        return 20;
    }

    if (fesetenv(&saved) != 0) {
        return 21;
    }
    if (puts("fenv-hyperbolic-interop-ok") == EOF) {
        return 22;
    }
    return 0;
}
