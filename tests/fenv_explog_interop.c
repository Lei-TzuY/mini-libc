#include <fenv.h>
#include <math.h>
#include <stdio.h>

#define MINI_FE_INVALID 0x01
#define MINI_FE_DIVBYZERO 0x04
#define MINI_FE_OVERFLOW 0x08
#define MINI_FE_UNDERFLOW 0x10
#define MINI_FE_INEXACT 0x20

double mini_test_exp(double x);
float mini_test_expf(float x);
double mini_test_log(double x);
float mini_test_logf(float x);

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

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 3;
    }
    value = mini_test_exp(1000.0);
    if (!isinf(value) || signbit(value) ||
        !has_flags(FE_OVERFLOW | FE_INEXACT)) {
        return 4;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        mini_test_exp(-1000.0) != 0.0 ||
        !has_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 5;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 6;
    }
    value = mini_test_log(0.0);
    if (!isinf(value) || !signbit(value) || !has_flags(FE_DIVBYZERO)) {
        return 7;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 8;
    }
    value = mini_test_log(-1.0);
    if (!isnan(value) || !has_flags(FE_INVALID)) {
        return 9;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        mini_test_exp(0.0) != 1.0 || fetestexcept(FE_ALL_EXCEPT) != 0) {
        return 10;
    }
    if (mini_test_exp(1.0) <= 2.7 || !has_flags(FE_INEXACT)) {
        return 11;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 || !isinf(mini_test_expf(100.0f)) ||
        !has_flags(FE_OVERFLOW | FE_INEXACT)) {
        return 12;
    }
    if (feclearexcept(FE_ALL_EXCEPT) != 0 || !isnan(mini_test_logf(-1.0f)) ||
        !has_flags(FE_INVALID)) {
        return 13;
    }

    if (fesetenv(&saved) != 0) {
        return 14;
    }
    if (puts("fenv-explog-interop-ok") == EOF) {
        return 15;
    }
    return 0;
}
