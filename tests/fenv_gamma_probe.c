#include <errno.h>
#include <fenv.h>
#include <math.h>
#include <mini/syscall.h>

static int has_flags(int flags)
{
    return (fetestexcept(flags) & flags) == flags;
}

static int clear_flags(void)
{
    return feclearexcept(FE_ALL_EXCEPT);
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

    if (fegetenv(&saved) != 0 || fesetenv(FE_DFL_ENV) != 0 ||
        clear_flags() != 0) {
        return 1;
    }

    errno = 71;
    value = tgamma(from_bits(0x7ff0000000000000ULL));
    if (!isinf(value) || signbit(value) ||
        !isinf(lgamma(from_bits(0x7ff0000000000000ULL))) ||
        !isinf(lgamma(from_bits(0xfff0000000000000ULL))) ||
        fetestexcept(FE_ALL_EXCEPT) != 0 || errno != 71) {
        return 2;
    }

    if (clear_flags() != 0) {
        return 3;
    }
    errno = 71;
    value = tgamma(0.0);
    if (!isinf(value) || signbit(value) || errno != ERANGE ||
        !has_flags(FE_DIVBYZERO) ||
        fetestexcept(FE_INVALID | FE_OVERFLOW | FE_UNDERFLOW) != 0) {
        return 4;
    }

    if (clear_flags() != 0) {
        return 5;
    }
    errno = 71;
    value = tgamma(from_bits(0x8000000000000000ULL));
    if (!isinf(value) || !signbit(value) || errno != ERANGE ||
        !has_flags(FE_DIVBYZERO)) {
        return 6;
    }

    if (clear_flags() != 0) {
        return 7;
    }
    errno = 71;
    value = lgamma(-2.0);
    if (!isinf(value) || signbit(value) || errno != ERANGE ||
        !has_flags(FE_DIVBYZERO) ||
        fetestexcept(FE_INVALID | FE_OVERFLOW | FE_UNDERFLOW) != 0) {
        return 8;
    }

    if (clear_flags() != 0) {
        return 9;
    }
    errno = 71;
    value = tgamma(-2.0);
    if (value == value || errno != EDOM || !has_flags(FE_INVALID) ||
        fetestexcept(FE_DIVBYZERO | FE_OVERFLOW | FE_UNDERFLOW) != 0) {
        return 10;
    }

    if (clear_flags() != 0) {
        return 11;
    }
    errno = 71;
    value = tgamma(from_bits(0xfff0000000000000ULL));
    if (value == value || errno != EDOM || !has_flags(FE_INVALID)) {
        return 12;
    }

    if (clear_flags() != 0) {
        return 13;
    }
    errno = 71;
    value = tgamma(172.0);
    if (!isinf(value) || signbit(value) || errno != ERANGE ||
        !has_flags(FE_OVERFLOW | FE_INEXACT)) {
        return 14;
    }

    if (clear_flags() != 0) {
        return 15;
    }
    errno = 71;
    value = tgamma(-180.5);
    if (value != 0.0 || (to_bits(value) & 0x8000000000000000ULL) == 0ULL ||
        errno != ERANGE || !has_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 16;
    }

    if (clear_flags() != 0) {
        return 17;
    }
    errno = 71;
    fvalue = tgammaf(36.0f);
    if (!isinf((double)fvalue) || errno != ERANGE ||
        !has_flags(FE_OVERFLOW | FE_INEXACT)) {
        return 18;
    }

    if (clear_flags() != 0) {
        return 19;
    }
    errno = 71;
    fvalue = tgammaf(-40.5f);
    if (fvalue != 0.0f || errno != ERANGE ||
        !has_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 20;
    }

    if (clear_flags() != 0) {
        return 21;
    }
    errno = 71;
    value = tgamma(0.5);
    if (!(value > 1.77 && value < 1.78) || errno != 71 ||
        !has_flags(FE_INEXACT) ||
        fetestexcept(FE_INVALID | FE_DIVBYZERO | FE_OVERFLOW | FE_UNDERFLOW) != 0) {
        return 22;
    }

    if (clear_flags() != 0 || feraiseexcept(FE_UNDERFLOW) != 0) {
        return 23;
    }
    errno = 71;
    if (!isinf(tgamma(from_bits(0x7ff0000000000000ULL))) ||
        !isinf(lgamma(from_bits(0xfff0000000000000ULL))) || errno != 71 ||
        fetestexcept(FE_ALL_EXCEPT) != FE_UNDERFLOW) {
        return 24;
    }

    if (fesetenv(&saved) != 0) {
        return 25;
    }
    if (mini_sys_write(1, "fenv-gamma-ok\n", 14) != 14) {
        return 26;
    }
    return 0;
}
