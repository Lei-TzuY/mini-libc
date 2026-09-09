#include <fenv.h>
#include <math.h>
#include <stdio.h>

double mini_test_ldexp(double x, int exp);
float mini_test_ldexpf(float x, int exp);
double mini_test_scalbn(double x, int n);
float mini_test_scalbnf(float x, int n);

static unsigned long long dbits(double value)
{
    union {
        double value;
        unsigned long long bits;
    } convert;

    convert.value = value;
    return convert.bits;
}

static unsigned int fbits(float value)
{
    union {
        float value;
        unsigned int bits;
    } convert;

    convert.value = value;
    return convert.bits;
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

static int only_flags(int flags)
{
    return fetestexcept(FE_ALL_EXCEPT) == flags;
}

int main(void)
{
    fenv_t saved;
    double value;
    float fvalue;

    if (fegetenv(&saved) != 0 || fesetenv(FE_DFL_ENV) != 0 ||
        feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 1;
    }

    if (mini_test_scalbn(0.75, 4) != 12.0 ||
        mini_test_ldexpf(0.75f, 4) != 12.0f || !only_flags(0)) {
        return 2;
    }

    value = mini_test_scalbn(1.0, -1074);
    fvalue = mini_test_scalbnf(1.0f, -149);
    if (dbits(value) != 1ULL || fbits(fvalue) != 1U || !only_flags(0)) {
        return 3;
    }

    if (fesetround(FE_UPWARD) != 0 || feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 16;
    }
    value = mini_test_scalbn(1.0, -1075);
    if (dbits(value) != 1ULL ||
        !only_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 17;
    }
    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 18;
    }
    value = mini_test_ldexp(-1.0, -1075);
    if (dbits(value) != 0x8000000000000000ULL ||
        !only_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 19;
    }
    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 20;
    }
    fvalue = mini_test_scalbnf(1.0f, -150);
    if (fbits(fvalue) != 1U ||
        !only_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 21;
    }

    if (fesetround(FE_DOWNWARD) != 0 || feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 22;
    }
    value = mini_test_scalbn(1.0, -1075);
    if (dbits(value) != 0ULL ||
        !only_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 23;
    }
    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 24;
    }
    value = mini_test_ldexp(-1.0, -1075);
    if (dbits(value) != 0x8000000000000001ULL ||
        !only_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 25;
    }
    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 26;
    }
    fvalue = mini_test_scalbnf(-1.0f, -150);
    if (fbits(fvalue) != 0x80000001U ||
        !only_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 27;
    }

    if (fesetround(FE_TOWARDZERO) != 0 ||
        feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 28;
    }
    value = mini_test_scalbn(1.5, -1074);
    if (dbits(value) != 1ULL ||
        !only_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 29;
    }
    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 30;
    }
    value = mini_test_scalbn(-1.5, -1074);
    if (dbits(value) != 0x8000000000000001ULL ||
        !only_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 31;
    }

    if (fesetround(FE_TONEAREST) != 0 || feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 32;
    }
    value = mini_test_scalbn(1.5, -1074);
    if (dbits(value) != 2ULL ||
        !only_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 33;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 14;
    }
    value = mini_test_scalbn(from_bits(0x3fffffffffffffffULL), -1023);
    if (dbits(value) != 0x0010000000000000ULL || !only_flags(FE_INEXACT)) {
        return 15;
    }

    if (fesetround(FE_DOWNWARD) != 0 || feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 34;
    }
    value = mini_test_scalbn(from_bits(0x3fffffffffffffffULL), -1023);
    if (dbits(value) != 0x000fffffffffffffULL ||
        !only_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 35;
    }
    if (fesetround(FE_UPWARD) != 0 || feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 36;
    }
    value = mini_test_scalbn(from_bits(0x3fffffffffffffffULL), -1023);
    if (dbits(value) != 0x0010000000000000ULL || !only_flags(FE_INEXACT)) {
        return 37;
    }
    if (fesetround(FE_TONEAREST) != 0 || feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 38;
    }

    value = mini_test_ldexp(1.0, 1024);
    if (!isinf(value) || !only_flags(FE_OVERFLOW | FE_INEXACT)) {
        return 5;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 6;
    }
    value = mini_test_scalbn(1.5, -1074);
    if (dbits(value) != 2ULL ||
        !only_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 7;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 8;
    }
    fvalue = mini_test_scalbnf(1.0f, -150);
    if (fbits(fvalue) != 0U ||
        !only_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 9;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        feraiseexcept(FE_DIVBYZERO) != 0) {
        return 10;
    }
    value = mini_test_scalbn(-1.0, 1024);
    if (!isinf(value) || !signbit(value) ||
        !only_flags(FE_DIVBYZERO | FE_OVERFLOW | FE_INEXACT)) {
        return 11;
    }

    if (fesetenv(&saved) != 0) {
        return 12;
    }
    if (puts("fenv-scaling-interop-ok") == EOF) {
        return 13;
    }
    return 0;
}
