#include <errno.h>
#include <fenv.h>
#include <math.h>
#include <mini/syscall.h>

#define MINI_LONG_MIN (-9223372036854775807L - 1L)
#define MINI_LLONG_MIN (-9223372036854775807LL - 1LL)

static double add_volatile(volatile double *left, volatile double *right)
{
    return *left + *right;
}

static double divide_volatile(volatile double *left, volatile double *right)
{
    return *left / *right;
}

static double quiet_nan(void)
{
    union {
        unsigned long long bits;
        double value;
    } convert;

    convert.bits = 0x7ff8000000000000ULL;
    return convert.value;
}

int main(void)
{
    fenv_t original;
    fenv_t held;
    fenv_t state;
    fexcept_t flags;
    volatile double one = 1.0;
    volatile double half_ulp = 0x1p-53;
    volatile double zero = 0.0;
    double value;

    errno = 71;
    if (fegetenv(&original) != 0 || errno != 71) {
        return 1;
    }
    if (fesetenv(FE_DFL_ENV) != 0 || fegetround() != FE_TONEAREST ||
        fetestexcept(FE_ALL_EXCEPT) != 0 || errno != 71) {
        return 2;
    }

    if (fesetround(FE_UPWARD) != 0 || fegetround() != FE_UPWARD) {
        return 3;
    }
    if (fegetenv(&state) != 0 ||
        (state.__x87_control & 0x0c00U) != FE_UPWARD ||
        (state.__mxcsr & 0x00006000U) != ((unsigned int)FE_UPWARD << 3)) {
        return 4;
    }
    value = add_volatile(&one, &half_ulp);
    if (!(value > 1.0)) {
        return 5;
    }

    if (fesetround(FE_DOWNWARD) != 0 || fegetround() != FE_DOWNWARD ||
        add_volatile(&one, &half_ulp) != 1.0) {
        return 6;
    }
    if (fesetround(1234) == 0 || fegetround() != FE_DOWNWARD) {
        return 7;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 8;
    }
    value = divide_volatile(&one, &zero);
    if (!(value > 1.0e308) ||
        (fetestexcept(FE_DIVBYZERO) & FE_DIVBYZERO) == 0) {
        return 9;
    }

    if (feraiseexcept(FE_INVALID | FE_INEXACT) != 0 ||
        (fetestexcept(FE_INVALID | FE_DIVBYZERO | FE_INEXACT) !=
         (FE_INVALID | FE_DIVBYZERO | FE_INEXACT))) {
        return 10;
    }
    if (fegetenv(&state) != 0 ||
        (state.__x87_exceptions & (FE_INVALID | FE_INEXACT)) !=
            (FE_INVALID | FE_INEXACT) ||
        (state.__mxcsr & (FE_INVALID | FE_INEXACT)) !=
            (FE_INVALID | FE_INEXACT)) {
        return 11;
    }

    if (fegetexceptflag(&flags, FE_INVALID | FE_INEXACT) != 0 ||
        flags != (FE_INVALID | FE_INEXACT)) {
        return 12;
    }
    if (feclearexcept(FE_INVALID | FE_INEXACT) != 0 ||
        fetestexcept(FE_INVALID | FE_INEXACT) != 0) {
        return 13;
    }
    if (fesetexceptflag(&flags, FE_INVALID | FE_INEXACT) != 0 ||
        fetestexcept(FE_INVALID | FE_INEXACT) !=
            (FE_INVALID | FE_INEXACT)) {
        return 14;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        fesetround(FE_DOWNWARD) != 0 ||
        feraiseexcept(FE_OVERFLOW) != 0) {
        return 15;
    }
    if (feholdexcept(&held) != 0 || fegetround() != FE_DOWNWARD ||
        fetestexcept(FE_ALL_EXCEPT) != 0) {
        return 16;
    }
    if (fegetenv(&state) != 0 ||
        (state.__x87_control & 0x003fU) != 0x003fU ||
        (state.__mxcsr & 0x00001f80U) != 0x00001f80U) {
        return 17;
    }
    if (feraiseexcept(FE_INVALID) != 0 || feupdateenv(&held) != 0 ||
        fegetround() != FE_DOWNWARD ||
        fetestexcept(FE_INVALID | FE_OVERFLOW) !=
            (FE_INVALID | FE_OVERFLOW)) {
        return 18;
    }

    if (fesetenv(FE_DFL_ENV) != 0 || feclearexcept(FE_ALL_EXCEPT) != 0 ||
        fesetround(FE_TONEAREST) != 0) {
        return 19;
    }
    errno = 71;
    if (rint(2.5) != 2.0 || rint(3.5) != 4.0 || rintf(-2.5f) != -2.0f ||
        (fetestexcept(FE_INEXACT) & FE_INEXACT) == 0 || errno != 71) {
        return 20;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        fesetround(FE_UPWARD) != 0 ||
        rint(1.25) != 2.0 || rintf(-1.25f) != -1.0f ||
        (fetestexcept(FE_INEXACT) & FE_INEXACT) == 0) {
        return 21;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        fesetround(FE_DOWNWARD) != 0 ||
        feraiseexcept(FE_OVERFLOW) != 0 ||
        nearbyint(-1.25) != -2.0 || nearbyintf(1.75f) != 1.0f ||
        fetestexcept(FE_ALL_EXCEPT) != FE_OVERFLOW) {
        return 22;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        fesetround(FE_TOWARDZERO) != 0 ||
        rint(-1.75) != -1.0 || rintf(1.75f) != 1.0f ||
        (fetestexcept(FE_INEXACT) & FE_INEXACT) == 0) {
        return 23;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        fesetround(FE_TONEAREST) != 0) {
        return 24;
    }
    errno = 71;
    if (lrint(2.5) != 2L || llrint(3.5) != 4LL || lrintf(-2.5f) != -2L ||
        llrintf(1.5f) != 2LL ||
        (fetestexcept(FE_INEXACT) & FE_INEXACT) == 0 || errno != 71) {
        return 25;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        fesetround(FE_DOWNWARD) != 0 ||
        llrint(-1.2) != -2LL ||
        (fetestexcept(FE_INEXACT) & FE_INEXACT) == 0) {
        return 26;
    }
    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        fesetround(FE_UPWARD) != 0 || lrint(1.2) != 2L ||
        (fetestexcept(FE_INEXACT) & FE_INEXACT) == 0) {
        return 27;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        fesetround(FE_TONEAREST) != 0) {
        return 28;
    }
    errno = 71;
    if (lrint(9223372036854775808.0) != MINI_LONG_MIN || errno != ERANGE ||
        (fetestexcept(FE_INVALID) & FE_INVALID) == 0) {
        return 29;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 30;
    }
    errno = 71;
    if (llrint(quiet_nan()) != MINI_LLONG_MIN || errno != EDOM ||
        (fetestexcept(FE_INVALID) & FE_INVALID) == 0) {
        return 31;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 || rint(4.0) != 4.0 ||
        fetestexcept(FE_INEXACT) != 0) {
        return 32;
    }

    if (fesetenv(&original) != 0) {
        return 33;
    }
    if (mini_sys_write(1, "fenv-ok\n", 8) != 8) {
        return 34;
    }
    return 0;
}
