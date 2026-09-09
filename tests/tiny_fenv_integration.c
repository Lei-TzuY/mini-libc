#include <errno.h>
#include <fenv.h>
#include <math.h>
#include <stdio.h>

#define MINI_LONG_MIN (-9223372036854775807L - 1L)

static double add_volatile(volatile double *left, volatile double *right)
{
    return *left + *right;
}

static double divide_volatile(volatile double *left, volatile double *right)
{
    return *left / *right;
}

static int has_flags(int flags)
{
    return (fetestexcept(flags) & flags) == flags;
}

int main(void)
{
    fenv_t saved;
    fenv_t state;
    volatile double one = 1.0;
    volatile double half_ulp = 1.11022302462515654042e-16;
    volatile double zero = 0.0;
    double value;
    float fvalue;
    int quotient;

    if (fegetenv(&saved) != 0 || fesetenv(FE_DFL_ENV) != 0) {
        return 1;
    }
    if (fesetround(FE_UPWARD) != 0 || fegetround() != FE_UPWARD) {
        return 2;
    }
    value = add_volatile(&one, &half_ulp);
    if (!(value > 1.0)) {
        return 3;
    }
    if (fegetenv(&state) != 0 ||
        (state.__x87_control & 0x0c00U) != FE_UPWARD ||
        (state.__mxcsr & 0x00006000U) != ((unsigned int)FE_UPWARD << 3)) {
        return 4;
    }

    if (fesetround(FE_DOWNWARD) != 0 ||
        add_volatile(&one, &half_ulp) != 1.0) {
        return 5;
    }
    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 6;
    }
    value = divide_volatile(&one, &zero);
    if (!(value > 1.0e308) ||
        (fetestexcept(FE_DIVBYZERO) & FE_DIVBYZERO) == 0) {
        return 7;
    }
    if (feraiseexcept(FE_INVALID | FE_INEXACT) != 0 ||
        fetestexcept(FE_INVALID | FE_DIVBYZERO | FE_INEXACT) !=
            (FE_INVALID | FE_DIVBYZERO | FE_INEXACT)) {
        return 8;
    }

    if (fesetenv(FE_DFL_ENV) != 0 || feclearexcept(FE_ALL_EXCEPT) != 0 ||
        rint(2.5) != 2.0 || rint(3.5) != 4.0 ||
        (fetestexcept(FE_INEXACT) & FE_INEXACT) == 0) {
        return 9;
    }
    if (feclearexcept(FE_ALL_EXCEPT) != 0 || fesetround(FE_UPWARD) != 0 ||
        nearbyint(1.25) != 2.0 || fetestexcept(FE_INEXACT) != 0) {
        return 10;
    }
    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        fesetround(FE_DOWNWARD) != 0 || lrint(-1.25) != -2L ||
        (fetestexcept(FE_INEXACT) & FE_INEXACT) == 0) {
        return 11;
    }
    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        fesetround(FE_TOWARDZERO) != 0 || rintf(-1.75f) != -1.0f ||
        (fetestexcept(FE_INEXACT) & FE_INEXACT) == 0) {
        return 12;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        fesetround(FE_TONEAREST) != 0) {
        return 13;
    }
    errno = 0;
    if (lrint(9223372036854775808.0) != MINI_LONG_MIN || errno != ERANGE ||
        (fetestexcept(FE_INVALID) & FE_INVALID) == 0) {
        return 14;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 15;
    }
    errno = 71;
    value = exp(1000.0);
    if (!isinf(value) || signbit(value) || errno != ERANGE ||
        (fetestexcept(FE_OVERFLOW | FE_INEXACT) &
         (FE_OVERFLOW | FE_INEXACT)) != (FE_OVERFLOW | FE_INEXACT)) {
        return 16;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 17;
    }
    errno = 71;
    if (exp(-1000.0) != 0.0 || errno != ERANGE ||
        (fetestexcept(FE_UNDERFLOW | FE_INEXACT) &
         (FE_UNDERFLOW | FE_INEXACT)) != (FE_UNDERFLOW | FE_INEXACT)) {
        return 18;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 19;
    }
    errno = 71;
    value = log(0.0);
    if (!isinf(value) || !signbit(value) || errno != ERANGE ||
        (fetestexcept(FE_DIVBYZERO) & FE_DIVBYZERO) == 0) {
        return 20;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 21;
    }
    errno = 71;
    value = log(-1.0);
    if (!isnan(value) || errno != EDOM ||
        (fetestexcept(FE_INVALID) & FE_INVALID) == 0) {
        return 22;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 23;
    }
    errno = 71;
    value = log(2.0);
    if (!(value > 0.69 && value < 0.70) || errno != 71 ||
        (fetestexcept(FE_INEXACT) & FE_INEXACT) == 0) {
        return 24;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 25;
    }
    errno = 71;
    if (pow(2.0, 3.0) != 8.0 || fetestexcept(FE_ALL_EXCEPT) != 0 ||
        errno != 71) {
        return 26;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 27;
    }
    errno = 71;
    value = pow(2.0, 0.5);
    if (!(value > 1.41 && value < 1.42) || errno != 71 ||
        !has_flags(FE_INEXACT)) {
        return 28;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 29;
    }
    errno = 71;
    value = pow(-2.0, 0.5);
    if (!isnan(value) || errno != EDOM || !has_flags(FE_INVALID)) {
        return 30;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 31;
    }
    errno = 71;
    value = pow(0.0, -3.0);
    if (!isinf(value) || signbit(value) || errno != ERANGE ||
        !has_flags(FE_DIVBYZERO)) {
        return 32;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 33;
    }
    errno = 71;
    value = pow(2.0, 1024.0);
    if (!isinf(value) || signbit(value) || errno != ERANGE ||
        !has_flags(FE_OVERFLOW | FE_INEXACT)) {
        return 34;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 35;
    }
    errno = 71;
    if (pow(2.0, -1075.0) != 0.0 || errno != ERANGE ||
        !has_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 36;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 37;
    }
    errno = 71;
    if (!isinf(powf(2.0f, 128.0f)) || errno != ERANGE ||
        !has_flags(FE_OVERFLOW | FE_INEXACT)) {
        return 38;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 39;
    }
    errno = 71;
    if (powf(2.0f, -150.0f) != 0.0f || errno != ERANGE ||
        !has_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 40;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 41;
    }
    errno = 71;
    if (sqrt(4.0) != 2.0 || fetestexcept(FE_ALL_EXCEPT) != 0 || errno != 71) {
        return 42;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 43;
    }
    errno = 71;
    value = sqrt(-1.0);
    if (!isnan(value) || errno != EDOM || !has_flags(FE_INVALID)) {
        return 44;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 45;
    }
    errno = 71;
    value = asin(1.5);
    if (!isnan(value) || errno != EDOM || !has_flags(FE_INVALID)) {
        return 46;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 47;
    }
    errno = 71;
    value = acos(-1.5);
    if (!isnan(value) || errno != EDOM || !has_flags(FE_INVALID)) {
        return 48;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 49;
    }
    errno = 71;
    value = acosh(0.5);
    if (!isnan(value) || errno != EDOM || !has_flags(FE_INVALID)) {
        return 50;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 51;
    }
    errno = 71;
    value = atanh(1.25);
    if (!isnan(value) || errno != EDOM || !has_flags(FE_INVALID)) {
        return 52;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 53;
    }
    errno = 71;
    value = atanh(1.0);
    if (!isinf(value) || signbit(value) || errno != ERANGE ||
        !has_flags(FE_DIVBYZERO)) {
        return 54;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 55;
    }
    errno = 71;
    value = atanh(0.5);
    if (!(value > 0.54 && value < 0.55) || errno != 71 ||
        !has_flags(FE_INEXACT)) {
        return 56;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 57;
    }
    errno = 72;
    quotient = 99;
    if (fmod(13.0, 4.0) != 1.0 || remainder(6.0, 4.0) != -2.0 ||
        remquo(30.0, 4.0, &quotient) != -2.0 || quotient != 8 ||
        errno != 72 || fetestexcept(FE_ALL_EXCEPT) != 0) {
        return 58;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 59;
    }
    errno = 73;
    value = fmod(1.0, 0.0);
    if (!isnan(value) || errno != EDOM || !has_flags(FE_INVALID)) {
        return 60;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        feraiseexcept(FE_OVERFLOW) != 0) {
        return 61;
    }
    errno = 74;
    quotient = 99;
    value = remquo(1.0, 0.0, &quotient);
    if (!isnan(value) || quotient != 0 || errno != EDOM ||
        !has_flags(FE_OVERFLOW | FE_INVALID)) {
        return 62;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 63;
    }
    errno = 75;
    fvalue = remainderf(1.0f, 0.0f);
    if (!isnan(fvalue) || errno != EDOM || !has_flags(FE_INVALID)) {
        return 64;
    }

    if (fesetenv(&saved) != 0) {
        return 65;
    }
    if (puts("tiny-fenv-ok") == EOF) {
        return 66;
    }
    return 0;
}
