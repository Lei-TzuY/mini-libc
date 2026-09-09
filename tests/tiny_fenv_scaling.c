#include <errno.h>
#include <fenv.h>
#include <math.h>
#include <stdio.h>

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

static int has_flags(int flags)
{
    return (fetestexcept(flags) & flags) == flags;
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

    errno = 71;
    if (scalbn(0.75, 4) != 12.0 || ldexpf(0.75f, 4) != 12.0f ||
        errno != 71 || fetestexcept(FE_ALL_EXCEPT) != 0) {
        return 2;
    }

    errno = 72;
    value = scalbn(1.0, -1074);
    fvalue = scalbnf(1.0f, -149);
    if (dbits(value) != 1ULL || fbits(fvalue) != 1U || errno != 72 ||
        fetestexcept(FE_ALL_EXCEPT) != 0) {
        return 3;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 4;
    }
    errno = 73;
    value = ldexp(1.0, 1024);
    if (!isinf(value) || errno != ERANGE ||
        !has_flags(FE_OVERFLOW | FE_INEXACT)) {
        return 5;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 6;
    }
    errno = 74;
    value = scalbn(1.5, -1074);
    if (dbits(value) != 2ULL || errno != ERANGE ||
        !has_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 7;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 8;
    }
    errno = 75;
    fvalue = scalbnf(1.0f, -150);
    if (fbits(fvalue) != 0U || errno != ERANGE ||
        !has_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 9;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        feraiseexcept(FE_DIVBYZERO) != 0) {
        return 10;
    }
    errno = 76;
    value = scalbn(-1.0, 1024);
    if (!isinf(value) || !signbit(value) || errno != ERANGE ||
        !has_flags(FE_DIVBYZERO | FE_OVERFLOW | FE_INEXACT)) {
        return 11;
    }

    if (fesetenv(&saved) != 0) {
        return 12;
    }
    if (puts("tiny-fenv-scaling-ok") == EOF) {
        return 13;
    }
    return 0;
}
