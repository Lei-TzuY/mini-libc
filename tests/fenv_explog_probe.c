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

int main(void)
{
    fenv_t saved;
    double value;

    if (fegetenv(&saved) != 0 || fesetenv(FE_DFL_ENV) != 0 ||
        feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 1;
    }

    errno = 71;
    if (exp(0.0) != 1.0 || fetestexcept(FE_ALL_EXCEPT) != 0 || errno != 71) {
        return 2;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 3;
    }
    errno = 71;
    value = exp(1.0);
    if (!(value > 2.7 && value < 2.8) ||
        (fetestexcept(FE_INEXACT) & FE_INEXACT) == 0 || errno != 71) {
        return 4;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 5;
    }
    errno = 71;
    value = exp(1000.0);
    if (!positive_infinity(value) || errno != ERANGE ||
        (fetestexcept(FE_OVERFLOW | FE_INEXACT) &
         (FE_OVERFLOW | FE_INEXACT)) != (FE_OVERFLOW | FE_INEXACT)) {
        return 6;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 7;
    }
    errno = 71;
    if (exp(-1000.0) != 0.0 || errno != ERANGE ||
        (fetestexcept(FE_UNDERFLOW | FE_INEXACT) &
         (FE_UNDERFLOW | FE_INEXACT)) != (FE_UNDERFLOW | FE_INEXACT)) {
        return 8;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 9;
    }
    errno = 71;
    if (log(1.0) != 0.0 || fetestexcept(FE_ALL_EXCEPT) != 0 || errno != 71) {
        return 10;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 11;
    }
    errno = 71;
    value = log(2.0);
    if (!(value > 0.69 && value < 0.70) ||
        (fetestexcept(FE_INEXACT) & FE_INEXACT) == 0 || errno != 71) {
        return 12;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 13;
    }
    errno = 71;
    value = log(0.0);
    if (!negative_infinity(value) || errno != ERANGE ||
        (fetestexcept(FE_DIVBYZERO) & FE_DIVBYZERO) == 0) {
        return 14;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 15;
    }
    errno = 71;
    value = log(-1.0);
    if (!isnan(value) || errno != EDOM ||
        (fetestexcept(FE_INVALID) & FE_INVALID) == 0) {
        return 16;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 17;
    }
    errno = 71;
    if (!isinf(expf(100.0f)) || errno != ERANGE ||
        (fetestexcept(FE_OVERFLOW | FE_INEXACT) &
         (FE_OVERFLOW | FE_INEXACT)) != (FE_OVERFLOW | FE_INEXACT)) {
        return 18;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 19;
    }
    errno = 71;
    if (expf(-200.0f) != 0.0f || errno != ERANGE ||
        (fetestexcept(FE_UNDERFLOW | FE_INEXACT) &
         (FE_UNDERFLOW | FE_INEXACT)) != (FE_UNDERFLOW | FE_INEXACT)) {
        return 20;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 21;
    }
    errno = 71;
    if (!isnan(logf(-1.0f)) || errno != EDOM ||
        (fetestexcept(FE_INVALID) & FE_INVALID) == 0) {
        return 22;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        feraiseexcept(FE_OVERFLOW) != 0) {
        return 23;
    }
    errno = 71;
    if (exp(0.0) != 1.0 || fetestexcept(FE_ALL_EXCEPT) != FE_OVERFLOW ||
        errno != 71) {
        return 24;
    }

    if (fesetenv(&saved) != 0) {
        return 25;
    }
    if (mini_sys_write(1, "fenv-explog-ok\n", 15) != 15) {
        return 26;
    }
    return 0;
}
