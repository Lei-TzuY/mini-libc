#include <fenv.h>
#include <math.h>
#include <stdio.h>

#define MINI_FE_INVALID 0x01
#define MINI_FE_DIVBYZERO 0x04
#define MINI_FE_INEXACT 0x20

double mini_test_sqrt(double x);
float mini_test_sqrtf(float x);
double mini_test_asin(double x);
float mini_test_asinf(float x);
double mini_test_acos(double x);
float mini_test_acosf(float x);
double mini_test_acosh(double x);
float mini_test_acoshf(float x);
double mini_test_atanh(double x);
float mini_test_atanhf(float x);

static int has_flags(int flags)
{
    return (fetestexcept(flags) & flags) == flags;
}

int main(void)
{
    fenv_t saved;
    double value;

    if (FE_INVALID != MINI_FE_INVALID || FE_DIVBYZERO != MINI_FE_DIVBYZERO ||
        FE_INEXACT != MINI_FE_INEXACT) {
        return 1;
    }
    if (fegetenv(&saved) != 0 || fesetenv(FE_DFL_ENV) != 0) {
        return 2;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 || mini_test_sqrt(4.0) != 2.0 ||
        fetestexcept(FE_ALL_EXCEPT) != 0) {
        return 3;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 4;
    }
    value = mini_test_sqrt(2.0);
    if (!(value > 1.41 && value < 1.42) || !has_flags(FE_INEXACT)) {
        return 5;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        !isnan(mini_test_sqrt(-1.0)) || !has_flags(FE_INVALID)) {
        return 6;
    }
    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        !isnan((double)mini_test_sqrtf(-1.0f)) || !has_flags(FE_INVALID)) {
        return 7;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 8;
    }
    value = mini_test_asin(0.5);
    if (!(value > 0.52 && value < 0.53) || !has_flags(FE_INEXACT)) {
        return 9;
    }
    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        !isnan(mini_test_asin(1.5)) || !has_flags(FE_INVALID)) {
        return 10;
    }
    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        !isnan((double)mini_test_asinf(1.5f)) || !has_flags(FE_INVALID)) {
        return 11;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 12;
    }
    value = mini_test_acos(0.5);
    if (!(value > 1.04 && value < 1.05) || !has_flags(FE_INEXACT)) {
        return 13;
    }
    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        !isnan(mini_test_acos(-1.5)) || !has_flags(FE_INVALID)) {
        return 14;
    }
    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        !isnan((double)mini_test_acosf(-1.5f)) || !has_flags(FE_INVALID)) {
        return 15;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 16;
    }
    value = mini_test_acosh(2.0);
    if (!(value > 1.31 && value < 1.32) || !has_flags(FE_INEXACT)) {
        return 17;
    }
    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        !isnan(mini_test_acosh(0.5)) || !has_flags(FE_INVALID)) {
        return 18;
    }
    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        !isnan((double)mini_test_acoshf(0.5f)) || !has_flags(FE_INVALID)) {
        return 19;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 20;
    }
    value = mini_test_atanh(0.5);
    if (!(value > 0.54 && value < 0.55) || !has_flags(FE_INEXACT)) {
        return 21;
    }
    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        !isnan(mini_test_atanh(1.25)) || !has_flags(FE_INVALID)) {
        return 22;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 23;
    }
    value = mini_test_atanh(1.0);
    if (!isinf(value) || signbit(value) || !has_flags(FE_DIVBYZERO)) {
        return 24;
    }
    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 25;
    }
    value = mini_test_atanh(-1.0);
    if (!isinf(value) || !signbit(value) || !has_flags(FE_DIVBYZERO)) {
        return 26;
    }
    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 27;
    }
    value = (double)mini_test_atanhf(1.0f);
    if (!isinf(value) || signbit(value) || !has_flags(FE_DIVBYZERO)) {
        return 28;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 || feraiseexcept(FE_INEXACT) != 0 ||
        mini_test_sqrt(4.0) != 2.0 || mini_test_asin(0.0) != 0.0 ||
        mini_test_acos(1.0) != 0.0 || mini_test_acosh(1.0) != 0.0 ||
        mini_test_atanh(0.0) != 0.0 ||
        fetestexcept(FE_ALL_EXCEPT) != FE_INEXACT) {
        return 29;
    }

    if (fesetenv(&saved) != 0) {
        return 30;
    }
    if (puts("fenv-domain-interop-ok") == EOF) {
        return 31;
    }
    return 0;
}
