#include <errno.h>
#include <fenv.h>
#include <math.h>
#include <stdio.h>

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
    if (sinh(0.0) != 0.0) {
        return 2;
    }
    if (cosh(0.0) != 1.0) {
        return 3;
    }
    if (tanh(0.0) != 0.0) {
        return 4;
    }
    if (fetestexcept(FE_ALL_EXCEPT) != 0 || errno != 71) {
        return 5;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 6;
    }
    errno = 71;
    value = sinh(711.0);
    if (!isinf(value) || signbit(value) || errno != ERANGE ||
        !has_flags(FE_OVERFLOW | FE_INEXACT)) {
        return 7;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 8;
    }
    errno = 71;
    value = cosh(711.0);
    if (!isinf(value) || signbit(value) || errno != ERANGE ||
        !has_flags(FE_OVERFLOW | FE_INEXACT)) {
        return 9;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 10;
    }
    errno = 71;
    value = (double)sinhf(90.0f);
    if (!isinf(value) || signbit(value) || errno != ERANGE ||
        !has_flags(FE_OVERFLOW | FE_INEXACT)) {
        return 11;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 12;
    }
    errno = 71;
    value = (double)coshf(90.0f);
    if (!isinf(value) || signbit(value) || errno != ERANGE ||
        !has_flags(FE_OVERFLOW | FE_INEXACT)) {
        return 13;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 14;
    }
    errno = 71;
    if (tanh(30.0) != 1.0 || tanhf(-30.0f) != -1.0f || errno != 71 ||
        !has_flags(FE_INEXACT) ||
        fetestexcept(FE_OVERFLOW | FE_UNDERFLOW | FE_DIVBYZERO | FE_INVALID) != 0) {
        return 15;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 || feraiseexcept(FE_UNDERFLOW) != 0) {
        return 16;
    }
    errno = 71;
    if (sinh(0.0) != 0.0 || cosh(0.0) != 1.0 || tanh(0.0) != 0.0 ||
        fetestexcept(FE_ALL_EXCEPT) != FE_UNDERFLOW || errno != 71) {
        return 17;
    }

    if (fesetenv(&saved) != 0) {
        return 18;
    }
    if (puts("tiny-fenv-hyperbolic-ok") == EOF) {
        return 19;
    }
    return 0;
}
