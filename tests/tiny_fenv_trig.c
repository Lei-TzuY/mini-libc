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

static int only_flag(int flag)
{
    return fetestexcept(FE_ALL_EXCEPT) == flag;
}

int main(void)
{
    fenv_t saved;
    double value;

    if (fegetenv(&saved) != 0 || fesetenv(FE_DFL_ENV) != 0 ||
        feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 1;
    }

    errno = 73;
    value = sin(-0.0);
    if (value != 0.0 || !signbit(value) || cos(0.0) != 1.0 ||
        tan(0.0) != 0.0 || fetestexcept(FE_ALL_EXCEPT) != 0 || errno != 73) {
        return 2;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 3;
    }
    errno = 73;
    value = sin(from_bits(0x7ff0000000000000ULL));
    if (!isnan(value) || errno != EDOM || !only_flag(FE_INVALID)) {
        return 4;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 5;
    }
    errno = 73;
    value = cos(1048577.0);
    if (!isnan(value) || errno != EDOM || !only_flag(FE_INVALID)) {
        return 6;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 7;
    }
    errno = 73;
    value = sin(0.5);
    if (!(value > 0.47 && value < 0.49) || errno != 73 ||
        !only_flag(FE_INEXACT)) {
        return 8;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 9;
    }
    errno = 73;
    value = cos(0.5);
    if (!(value > 0.87 && value < 0.89) || errno != 73 ||
        !only_flag(FE_INEXACT)) {
        return 10;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 11;
    }
    errno = 73;
    value = tan(0.5);
    if (!(value > 0.54 && value < 0.55) || errno != 73 ||
        !only_flag(FE_INEXACT)) {
        return 12;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 13;
    }
    errno = 73;
    value = (double)sinf(0.5f);
    if (!(value > 0.47 && value < 0.49) || errno != 73 ||
        !only_flag(FE_INEXACT)) {
        return 14;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 || feraiseexcept(FE_UNDERFLOW) != 0) {
        return 15;
    }
    errno = 73;
    if (sin(0.0) != 0.0 || cos(0.0) != 1.0 || tan(0.0) != 0.0 ||
        fetestexcept(FE_ALL_EXCEPT) != FE_UNDERFLOW || errno != 73) {
        return 16;
    }

    if (fesetenv(&saved) != 0) {
        return 17;
    }
    if (puts("tiny-fenv-trig-ok") == EOF) {
        return 18;
    }
    return 0;
}
