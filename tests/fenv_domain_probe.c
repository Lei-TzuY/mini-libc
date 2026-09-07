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

int main(void)
{
    fenv_t saved;
    double value;

    if (fegetenv(&saved) != 0 || fesetenv(FE_DFL_ENV) != 0 ||
        clear_flags() != 0) {
        return 1;
    }

    errno = 71;
    if (sqrt(4.0) != 2.0 || fetestexcept(FE_ALL_EXCEPT) != 0 || errno != 71) {
        return 2;
    }

    if (clear_flags() != 0) {
        return 3;
    }
    errno = 71;
    value = sqrt(-0.0);
    if (value != 0.0 || !signbit(value) || fetestexcept(FE_ALL_EXCEPT) != 0 ||
        errno != 71) {
        return 4;
    }

    if (clear_flags() != 0) {
        return 5;
    }
    errno = 71;
    value = sqrt(2.0);
    if (!(value > 1.41 && value < 1.42) || errno != 71 ||
        !has_flags(FE_INEXACT)) {
        return 6;
    }

    if (clear_flags() != 0) {
        return 7;
    }
    errno = 71;
    value = sqrt(-1.0);
    if (!isnan(value) || errno != EDOM || !has_flags(FE_INVALID)) {
        return 8;
    }

    if (clear_flags() != 0) {
        return 9;
    }
    errno = 71;
    if (!isnan(sqrtf(-1.0f)) || errno != EDOM || !has_flags(FE_INVALID)) {
        return 10;
    }

    if (clear_flags() != 0) {
        return 11;
    }
    errno = 71;
    if (asin(0.0) != 0.0 || fetestexcept(FE_ALL_EXCEPT) != 0 || errno != 71) {
        return 12;
    }

    if (clear_flags() != 0) {
        return 13;
    }
    errno = 71;
    value = asin(0.5);
    if (!(value > 0.52 && value < 0.53) || errno != 71 ||
        !has_flags(FE_INEXACT)) {
        return 14;
    }

    if (clear_flags() != 0) {
        return 15;
    }
    errno = 71;
    value = asin(1.5);
    if (!isnan(value) || errno != EDOM || !has_flags(FE_INVALID)) {
        return 16;
    }

    if (clear_flags() != 0) {
        return 17;
    }
    errno = 71;
    if (acos(1.0) != 0.0 || fetestexcept(FE_ALL_EXCEPT) != 0 || errno != 71) {
        return 18;
    }

    if (clear_flags() != 0) {
        return 19;
    }
    errno = 71;
    value = acos(0.5);
    if (!(value > 1.04 && value < 1.05) || errno != 71 ||
        !has_flags(FE_INEXACT)) {
        return 20;
    }

    if (clear_flags() != 0) {
        return 21;
    }
    errno = 71;
    value = acos(-1.5);
    if (!isnan(value) || errno != EDOM || !has_flags(FE_INVALID)) {
        return 22;
    }

    if (clear_flags() != 0) {
        return 23;
    }
    errno = 71;
    if (acosh(1.0) != 0.0 || fetestexcept(FE_ALL_EXCEPT) != 0 || errno != 71) {
        return 24;
    }

    if (clear_flags() != 0) {
        return 25;
    }
    errno = 71;
    value = acosh(2.0);
    if (!(value > 1.31 && value < 1.32) || errno != 71 ||
        !has_flags(FE_INEXACT)) {
        return 26;
    }

    if (clear_flags() != 0) {
        return 27;
    }
    errno = 71;
    value = acosh(0.5);
    if (!isnan(value) || errno != EDOM || !has_flags(FE_INVALID)) {
        return 28;
    }

    if (clear_flags() != 0) {
        return 29;
    }
    errno = 71;
    if (atanh(0.0) != 0.0 || fetestexcept(FE_ALL_EXCEPT) != 0 || errno != 71) {
        return 30;
    }

    if (clear_flags() != 0) {
        return 31;
    }
    errno = 71;
    value = atanh(0.5);
    if (!(value > 0.54 && value < 0.55) || errno != 71 ||
        !has_flags(FE_INEXACT)) {
        return 32;
    }

    if (clear_flags() != 0) {
        return 33;
    }
    errno = 71;
    value = atanh(1.25);
    if (!isnan(value) || errno != EDOM || !has_flags(FE_INVALID)) {
        return 34;
    }

    if (clear_flags() != 0) {
        return 35;
    }
    errno = 71;
    value = atanh(1.0);
    if (!isinf(value) || signbit(value) || errno != ERANGE ||
        !has_flags(FE_DIVBYZERO)) {
        return 36;
    }

    if (clear_flags() != 0) {
        return 37;
    }
    errno = 71;
    value = atanh(-1.0);
    if (!isinf(value) || !signbit(value) || errno != ERANGE ||
        !has_flags(FE_DIVBYZERO)) {
        return 38;
    }

    if (clear_flags() != 0) {
        return 39;
    }
    errno = 71;
    if (!isnan(asinf(1.5f)) || errno != EDOM || !has_flags(FE_INVALID)) {
        return 40;
    }

    if (clear_flags() != 0) {
        return 41;
    }
    errno = 71;
    if (!isnan(acosf(-1.5f)) || errno != EDOM || !has_flags(FE_INVALID)) {
        return 42;
    }

    if (clear_flags() != 0) {
        return 43;
    }
    errno = 71;
    if (!isnan(acoshf(0.5f)) || errno != EDOM || !has_flags(FE_INVALID)) {
        return 44;
    }

    if (clear_flags() != 0) {
        return 45;
    }
    errno = 71;
    value = (double)atanhf(1.0f);
    if (!isinf(value) || signbit(value) || errno != ERANGE ||
        !has_flags(FE_DIVBYZERO)) {
        return 46;
    }

    if (clear_flags() != 0 || feraiseexcept(FE_OVERFLOW) != 0) {
        return 47;
    }
    errno = 71;
    if (sqrt(4.0) != 2.0 || asin(0.0) != 0.0 || acos(1.0) != 0.0 ||
        acosh(1.0) != 0.0 || atanh(0.0) != 0.0 ||
        fetestexcept(FE_ALL_EXCEPT) != FE_OVERFLOW || errno != 71) {
        return 48;
    }

    if (fesetenv(&saved) != 0) {
        return 49;
    }
    if (mini_sys_write(1, "fenv-domain-ok\n", 15) != 15) {
        return 50;
    }
    return 0;
}
