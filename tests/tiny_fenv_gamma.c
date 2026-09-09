#include <errno.h>
#include <fenv.h>
#include <math.h>
#include <stdio.h>

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

int main(void)
{
    double value;
    float fvalue;

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 1;
    }
    errno = 71;
    value = tgamma(0.0);
    if (!isinf(value) || signbit(value) || errno != ERANGE ||
        !has_flags(FE_DIVBYZERO)) {
        return 2;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 3;
    }
    errno = 71;
    value = tgamma(-2.0);
    if (value == value || errno != EDOM || !has_flags(FE_INVALID)) {
        return 4;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 5;
    }
    errno = 71;
    value = lgamma(-2.0);
    if (!isinf(value) || errno != ERANGE || !has_flags(FE_DIVBYZERO)) {
        return 6;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 7;
    }
    errno = 71;
    value = tgamma(172.0);
    if (!isinf(value) || errno != ERANGE ||
        !has_flags(FE_OVERFLOW | FE_INEXACT)) {
        return 8;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 9;
    }
    errno = 71;
    value = tgamma(-180.5);
    if (value != 0.0 || errno != ERANGE ||
        !has_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 10;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 11;
    }
    errno = 71;
    fvalue = tgammaf(36.0f);
    if (!isinf((double)fvalue) || errno != ERANGE ||
        !has_flags(FE_OVERFLOW | FE_INEXACT)) {
        return 12;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 13;
    }
    errno = 71;
    value = tgamma(0.5);
    if (!(value > 1.77 && value < 1.78) || errno != 71 ||
        !has_flags(FE_INEXACT)) {
        return 14;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 || feraiseexcept(FE_UNDERFLOW) != 0) {
        return 15;
    }
    errno = 71;
    if (!isinf(tgamma(from_bits(0x7ff0000000000000ULL))) || errno != 71 ||
        fetestexcept(FE_ALL_EXCEPT) != FE_UNDERFLOW) {
        return 16;
    }

    if (puts("tiny-fenv-gamma-ok") == EOF) {
        return 17;
    }
    return 0;
}
