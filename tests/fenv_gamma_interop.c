#include <fenv.h>
#include <math.h>
#include <stdio.h>

double mini_test_tgamma(double x);
float mini_test_tgammaf(float x);
double mini_test_lgamma(double x);
float mini_test_lgammaf(float x);

static int has_flags(int flags)
{
    return (fetestexcept(flags) & flags) == flags;
}

static double from_bits(unsigned long long bits)
{
    union {
        double value;
        unsigned long long bits;
    } convert;

    convert.bits = bits;
    return convert.value;
}

static unsigned long long to_bits(double value)
{
    union {
        double value;
        unsigned long long bits;
    } convert;

    convert.value = value;
    return convert.bits;
}

int main(void)
{
    fenv_t saved;
    double value;
    float fvalue;

    if (fegetenv(&saved) != 0 || fesetenv(FE_DFL_ENV) != 0) {
        return 1;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 2;
    }
    value = mini_test_tgamma(0.0);
    if (!isinf(value) || signbit(value) || !has_flags(FE_DIVBYZERO) ||
        fetestexcept(FE_INVALID | FE_OVERFLOW | FE_UNDERFLOW) != 0) {
        return 3;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 4;
    }
    value = mini_test_lgamma(-2.0);
    if (!isinf(value) || signbit(value) || !has_flags(FE_DIVBYZERO)) {
        return 5;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 6;
    }
    value = mini_test_tgamma(-2.0);
    if (value == value || !has_flags(FE_INVALID)) {
        return 7;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 8;
    }
    value = mini_test_tgamma(from_bits(0xfff0000000000000ULL));
    if (value == value || !has_flags(FE_INVALID)) {
        return 9;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 10;
    }
    value = mini_test_tgamma(172.0);
    if (!isinf(value) || !has_flags(FE_OVERFLOW | FE_INEXACT)) {
        return 11;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 12;
    }
    value = mini_test_tgamma(-180.5);
    if (value != 0.0 || (to_bits(value) & 0x8000000000000000ULL) == 0ULL ||
        !has_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 13;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 14;
    }
    fvalue = mini_test_tgammaf(36.0f);
    if (!isinf((double)fvalue) || !has_flags(FE_OVERFLOW | FE_INEXACT)) {
        return 15;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 16;
    }
    fvalue = mini_test_tgammaf(-40.5f);
    if (fvalue != 0.0f || !has_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 17;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 18;
    }
    value = mini_test_tgamma(0.5);
    if (!(value > 1.77 && value < 1.78) || !has_flags(FE_INEXACT) ||
        fetestexcept(FE_INVALID | FE_DIVBYZERO | FE_OVERFLOW | FE_UNDERFLOW) != 0) {
        return 19;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 || feraiseexcept(FE_UNDERFLOW) != 0) {
        return 20;
    }
    if (!isinf(mini_test_tgamma(from_bits(0x7ff0000000000000ULL))) ||
        !isinf(mini_test_lgamma(from_bits(0xfff0000000000000ULL))) ||
        fetestexcept(FE_ALL_EXCEPT) != FE_UNDERFLOW) {
        return 21;
    }

    if (fesetenv(&saved) != 0) {
        return 22;
    }
    if (puts("fenv-gamma-interop-ok") == EOF) {
        return 23;
    }
    return 0;
}
