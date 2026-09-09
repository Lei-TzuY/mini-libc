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
    } value;

    value.bits = bits;
    return value.value;
}

int main(void)
{
    fenv_t saved;
    double value;

    if (fegetenv(&saved) != 0 || fesetenv(FE_DFL_ENV) != 0 ||
        clear_flags() != 0) {
        return 1;
    }

    errno = 71;
    value = sinh(-0.0);
    if (value != 0.0 || !signbit(value) || cosh(0.0) != 1.0 ||
        tanh(0.0) != 0.0 || fetestexcept(FE_ALL_EXCEPT) != 0 || errno != 71) {
        return 2;
    }

    if (clear_flags() != 0) {
        return 3;
    }
    errno = 71;
    value = sinh(from_bits(0x7ff0000000000000ULL));
    if (!isinf(value) || signbit(value) ||
        !isinf(cosh(from_bits(0xfff0000000000000ULL))) ||
        tanh(from_bits(0x7ff0000000000000ULL)) != 1.0 ||
        tanh(from_bits(0xfff0000000000000ULL)) != -1.0 ||
        fetestexcept(FE_ALL_EXCEPT) != 0 || errno != 71) {
        return 4;
    }

    if (clear_flags() != 0) {
        return 5;
    }
    errno = 71;
    value = sinh(1.0);
    if (!(value > 1.17 && value < 1.18) || errno != 71 ||
        !has_flags(FE_INEXACT)) {
        return 6;
    }

    if (clear_flags() != 0) {
        return 7;
    }
    errno = 71;
    value = cosh(1.0);
    if (!(value > 1.54 && value < 1.55) || errno != 71 ||
        !has_flags(FE_INEXACT)) {
        return 8;
    }

    if (clear_flags() != 0) {
        return 9;
    }
    errno = 71;
    value = tanh(1.0);
    if (!(value > 0.76 && value < 0.77) || errno != 71 ||
        !has_flags(FE_INEXACT)) {
        return 10;
    }

    if (clear_flags() != 0) {
        return 11;
    }
    errno = 71;
    value = sinh(711.0);
    if (!isinf(value) || signbit(value) || errno != ERANGE ||
        !has_flags(FE_OVERFLOW | FE_INEXACT)) {
        return 12;
    }

    if (clear_flags() != 0) {
        return 13;
    }
    errno = 71;
    value = sinh(-711.0);
    if (!isinf(value) || !signbit(value) || errno != ERANGE ||
        !has_flags(FE_OVERFLOW | FE_INEXACT)) {
        return 14;
    }

    if (clear_flags() != 0) {
        return 15;
    }
    errno = 71;
    value = cosh(711.0);
    if (!isinf(value) || signbit(value) || errno != ERANGE ||
        !has_flags(FE_OVERFLOW | FE_INEXACT)) {
        return 16;
    }

    if (clear_flags() != 0) {
        return 17;
    }
    errno = 71;
    value = (double)sinhf(90.0f);
    if (!isinf(value) || signbit(value) || errno != ERANGE ||
        !has_flags(FE_OVERFLOW | FE_INEXACT)) {
        return 18;
    }

    if (clear_flags() != 0) {
        return 19;
    }
    errno = 71;
    value = (double)coshf(90.0f);
    if (!isinf(value) || signbit(value) || errno != ERANGE ||
        !has_flags(FE_OVERFLOW | FE_INEXACT)) {
        return 20;
    }

    if (clear_flags() != 0) {
        return 21;
    }
    errno = 71;
    if (tanh(30.0) != 1.0 || errno != 71 || !has_flags(FE_INEXACT) ||
        (fetestexcept(FE_OVERFLOW | FE_UNDERFLOW | FE_DIVBYZERO | FE_INVALID) != 0)) {
        return 22;
    }

    if (clear_flags() != 0) {
        return 23;
    }
    errno = 71;
    if (tanhf(-30.0f) != -1.0f || errno != 71 || !has_flags(FE_INEXACT) ||
        (fetestexcept(FE_OVERFLOW | FE_UNDERFLOW | FE_DIVBYZERO | FE_INVALID) != 0)) {
        return 24;
    }

    if (clear_flags() != 0 || feraiseexcept(FE_UNDERFLOW) != 0) {
        return 25;
    }
    errno = 71;
    if (sinh(0.0) != 0.0 || cosh(0.0) != 1.0 || tanh(0.0) != 0.0 ||
        fetestexcept(FE_ALL_EXCEPT) != FE_UNDERFLOW || errno != 71) {
        return 26;
    }

    if (fesetenv(&saved) != 0) {
        return 27;
    }
    if (mini_sys_write(1, "fenv-hyperbolic-ok\n", 19) != 19) {
        return 28;
    }
    return 0;
}
