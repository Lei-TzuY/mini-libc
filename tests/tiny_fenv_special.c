#include <errno.h>
#include <fenv.h>
#include <math.h>
#include <stdio.h>

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

    errno = 73;
    value = erf(-0.0);
    if (value != 0.0 || !signbit(value) || erfc(0.0) != 1.0 ||
        erf(from_bits(0x7ff0000000000000ULL)) != 1.0 ||
        fetestexcept(FE_ALL_EXCEPT) != 0 || errno != 73) {
        return 2;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 3;
    }
    errno = 74;
    value = erf(0.5);
    if (!(value > 0.52 && value < 0.521) || errno != 74 ||
        !only_flags(FE_INEXACT)) {
        return 4;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 5;
    }
    errno = 75;
    value = erfc(30.0);
    if (value != 0.0 || errno != ERANGE ||
        !only_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 6;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 7;
    }
    errno = 76;
    value = erfc(-30.0);
    if (value != 2.0 || errno != 76 || !only_flags(FE_INEXACT)) {
        return 8;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 9;
    }
    errno = 77;
    fvalue = erfcf(12.0f);
    if (fvalue != 0.0f || errno != ERANGE ||
        !only_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 10;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        feraiseexcept(FE_DIVBYZERO) != 0) {
        return 11;
    }
    errno = 78;
    value = erfc(-30.0);
    if (value != 2.0 || errno != 78 ||
        !only_flags(FE_DIVBYZERO | FE_INEXACT)) {
        return 12;
    }

    if (fesetenv(&saved) != 0) {
        return 13;
    }
    if (puts("tiny-fenv-special-ok") == EOF) {
        return 14;
    }
    return 0;
}
