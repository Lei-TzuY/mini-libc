#include <errno.h>
#include <fenv.h>
#include <math.h>
#include <mini/syscall.h>

static int positive_infinity(double x)
{
    return isinf(x) && !signbit(x);
}

static int negative_infinity(double x)
{
    return isinf(x) && signbit(x);
}

static int has_flags(int flags)
{
    return (fetestexcept(flags) & flags) == flags;
}

int main(void)
{
    fenv_t saved;
    double value;

    if (fegetenv(&saved) != 0 || fesetenv(FE_DFL_ENV) != 0 ||
        feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 1;
    }

    errno = 71;
    if (pow(2.0, 3.0) != 8.0 || fetestexcept(FE_ALL_EXCEPT) != 0 ||
        errno != 71) {
        return 2;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 3;
    }
    errno = 71;
    if (pow(-2.0, 3.0) != -8.0 || fetestexcept(FE_ALL_EXCEPT) != 0 ||
        errno != 71) {
        return 4;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 5;
    }
    errno = 71;
    if (pow(2.0, -3.0) != 0.125 || fetestexcept(FE_ALL_EXCEPT) != 0 ||
        errno != 71) {
        return 6;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 7;
    }
    errno = 71;
    value = pow(2.0, 0.5);
    if (!(value > 1.41 && value < 1.42) || errno != 71 ||
        !has_flags(FE_INEXACT)) {
        return 8;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 9;
    }
    errno = 71;
    value = pow(-2.0, 0.5);
    if (!isnan(value) || errno != EDOM || !has_flags(FE_INVALID)) {
        return 10;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 11;
    }
    errno = 71;
    value = pow(0.0, -3.0);
    if (!positive_infinity(value) || errno != ERANGE ||
        !has_flags(FE_DIVBYZERO)) {
        return 12;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 13;
    }
    errno = 71;
    value = pow(-0.0, -3.0);
    if (!negative_infinity(value) || errno != ERANGE ||
        !has_flags(FE_DIVBYZERO)) {
        return 14;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 15;
    }
    errno = 71;
    value = pow(2.0, 1024.0);
    if (!positive_infinity(value) || errno != ERANGE ||
        !has_flags(FE_OVERFLOW | FE_INEXACT)) {
        return 16;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 17;
    }
    errno = 71;
    value = pow(2.0, -1075.0);
    if (value != 0.0 || errno != ERANGE ||
        !has_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 18;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 19;
    }
    errno = 71;
    if (!isinf(powf(2.0f, 128.0f)) || errno != ERANGE ||
        !has_flags(FE_OVERFLOW | FE_INEXACT)) {
        return 20;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 21;
    }
    errno = 71;
    if (powf(2.0f, -150.0f) != 0.0f || errno != ERANGE ||
        !has_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 22;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        feraiseexcept(FE_OVERFLOW) != 0) {
        return 23;
    }
    errno = 71;
    if (pow(2.0, 3.0) != 8.0 || fetestexcept(FE_ALL_EXCEPT) != FE_OVERFLOW ||
        errno != 71) {
        return 24;
    }

    if (fesetenv(&saved) != 0) {
        return 25;
    }
    if (mini_sys_write(1, "fenv-pow-ok\n", 12) != 12) {
        return 26;
    }
    return 0;
}
