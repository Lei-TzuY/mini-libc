#include <errno.h>
#include <fenv.h>
#include <math.h>
#include <mini/syscall.h>

static unsigned long long dbits(double value)
{
    union {
        double value;
        unsigned long long bits;
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

static int clear_flags(void)
{
    return feclearexcept(FE_ALL_EXCEPT);
}

static int only_flag(int flag)
{
    return fetestexcept(FE_ALL_EXCEPT) == flag;
}

int main(void)
{
    fenv_t saved;
    double value;
    double nan_value = from_bits(0x7ff8123456789abcULL);

    if (fegetenv(&saved) != 0 || fesetenv(FE_DFL_ENV) != 0 ||
        clear_flags() != 0) {
        return 1;
    }

    errno = 73;
    if (dbits(sin(-0.0)) != 0x8000000000000000ULL ||
        dbits(tan(-0.0)) != 0x8000000000000000ULL || cos(-0.0) != 1.0 ||
        fetestexcept(FE_ALL_EXCEPT) != 0 || errno != 73) {
        return 2;
    }

    if (clear_flags() != 0) {
        return 3;
    }
    errno = 73;
    value = sin(from_bits(0x7ff0000000000000ULL));
    if (!isnan(value) || errno != EDOM || !only_flag(FE_INVALID)) {
        return 4;
    }

    if (clear_flags() != 0) {
        return 5;
    }
    errno = 73;
    value = cos(1048577.0);
    if (!isnan(value) || errno != EDOM || !only_flag(FE_INVALID)) {
        return 6;
    }

    if (clear_flags() != 0) {
        return 7;
    }
    errno = 73;
    value = tan(from_bits(0xfff0000000000000ULL));
    if (!isnan(value) || errno != EDOM || !only_flag(FE_INVALID)) {
        return 8;
    }

    if (clear_flags() != 0) {
        return 9;
    }
    errno = 73;
    value = sin(0.5);
    if (!(value > 0.47 && value < 0.49) || errno != 73 ||
        !only_flag(FE_INEXACT)) {
        return 10;
    }

    if (clear_flags() != 0) {
        return 11;
    }
    errno = 73;
    value = cos(0.5);
    if (!(value > 0.87 && value < 0.89) || errno != 73 ||
        !only_flag(FE_INEXACT)) {
        return 12;
    }

    if (clear_flags() != 0) {
        return 13;
    }
    errno = 73;
    value = tan(0.5);
    if (!(value > 0.54 && value < 0.55) || errno != 73 ||
        !only_flag(FE_INEXACT)) {
        return 14;
    }

    if (clear_flags() != 0) {
        return 15;
    }
    errno = 73;
    value = (double)sinf(0.5f);
    if (!(value > 0.47 && value < 0.49) || errno != 73 ||
        !only_flag(FE_INEXACT)) {
        return 16;
    }

    if (clear_flags() != 0 || feraiseexcept(FE_UNDERFLOW) != 0) {
        return 17;
    }
    errno = 73;
    if (dbits(sin(nan_value)) != 0x7ff8123456789abcULL ||
        sin(0.0) != 0.0 || cos(0.0) != 1.0 || tan(0.0) != 0.0 ||
        fetestexcept(FE_ALL_EXCEPT) != FE_UNDERFLOW || errno != 73) {
        return 18;
    }

    if (fesetenv(&saved) != 0) {
        return 19;
    }
    if (mini_sys_write(1, "fenv-trig-ok\n", 13) != 13) {
        return 20;
    }
    return 0;
}
