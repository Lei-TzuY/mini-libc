#include <fenv.h>
#include <math.h>
#include <stdio.h>

double mini_test_erf(double x);
float mini_test_erff(float x);
double mini_test_erfc(double x);
float mini_test_erfcf(float x);

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

    if (fegetenv(&saved) != 0 || fesetenv(FE_DFL_ENV) != 0) {
        return 1;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 2;
    }
    if (mini_test_erf(-0.0) != 0.0 || mini_test_erfc(0.0) != 1.0 ||
        mini_test_erf(from_bits(0x7ff0000000000000ULL)) != 1.0 ||
        mini_test_erfc(from_bits(0xfff0000000000000ULL)) != 2.0 ||
        !only_flags(0)) {
        return 3;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 4;
    }
    value = mini_test_erf(0.5);
    if (!(value > 0.52 && value < 0.521) || !only_flags(FE_INEXACT)) {
        return 5;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 6;
    }
    value = mini_test_erfc(30.0);
    if (value != 0.0 || !only_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 7;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 8;
    }
    value = mini_test_erfc(-30.0);
    if (value != 2.0 || !only_flags(FE_INEXACT)) {
        return 9;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 10;
    }
    fvalue = mini_test_erfcf(12.0f);
    if (fvalue != 0.0f || !only_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 11;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        feraiseexcept(FE_DIVBYZERO) != 0) {
        return 12;
    }
    value = mini_test_erfc(-30.0);
    if (value != 2.0 || !only_flags(FE_DIVBYZERO | FE_INEXACT)) {
        return 13;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        feraiseexcept(FE_OVERFLOW) != 0) {
        return 14;
    }
    value = mini_test_erf(from_bits(0x7ff8123456789abcULL));
    if (!isnan(value) || !only_flags(FE_OVERFLOW)) {
        return 15;
    }

    if (fesetenv(&saved) != 0) {
        return 16;
    }
    if (puts("fenv-special-interop-ok") == EOF) {
        return 17;
    }
    return 0;
}
