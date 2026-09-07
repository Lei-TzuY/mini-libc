#include <fenv.h>
#include <stdio.h>

#define MINI_FE_INVALID 0x01
#define MINI_FE_OVERFLOW 0x08
#define MINI_FE_INEXACT 0x20
#define MINI_FE_TONEAREST 0x0000
#define MINI_FE_DOWNWARD 0x0400
#define MINI_FE_UPWARD 0x0800
#define MINI_FE_TOWARDZERO 0x0c00
#define MINI_LONG_MIN (-9223372036854775807L - 1L)
#define MINI_LLONG_MIN (-9223372036854775807LL - 1LL)

double mini_test_rint(double x);
float mini_test_rintf(float x);
double mini_test_nearbyint(double x);
float mini_test_nearbyintf(float x);
long mini_test_lrint(double x);
long mini_test_lrintf(float x);
long long mini_test_llrint(double x);
long long mini_test_llrintf(float x);

int mini_test_feclearexcept(int excepts);
int mini_test_feraiseexcept(int excepts);
int mini_test_fetestexcept(int excepts);
int mini_test_fegetround(void);
int mini_test_fesetround(int round);

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
    fenv_t saved;

    if (FE_INVALID != MINI_FE_INVALID || FE_OVERFLOW != MINI_FE_OVERFLOW ||
        FE_INEXACT != MINI_FE_INEXACT || FE_TONEAREST != MINI_FE_TONEAREST ||
        FE_DOWNWARD != MINI_FE_DOWNWARD || FE_UPWARD != MINI_FE_UPWARD ||
        FE_TOWARDZERO != MINI_FE_TOWARDZERO) {
        return 1;
    }
    if (fegetenv(&saved) != 0 || fesetenv(FE_DFL_ENV) != 0) {
        return 2;
    }

    if (mini_test_fesetround(MINI_FE_UPWARD) != 0 ||
        fegetround() != FE_UPWARD || feclearexcept(FE_ALL_EXCEPT) != 0 ||
        mini_test_rint(1.25) != 2.0 || mini_test_rintf(-1.25f) != -1.0f ||
        (fetestexcept(FE_INEXACT) & FE_INEXACT) == 0) {
        return 3;
    }

    if (mini_test_feclearexcept(MINI_FE_INEXACT | MINI_FE_OVERFLOW) != 0 ||
        mini_test_feraiseexcept(MINI_FE_OVERFLOW) != 0 ||
        mini_test_nearbyint(1.25) != 2.0 ||
        mini_test_nearbyintf(-1.25f) != -1.0f ||
        fetestexcept(FE_INEXACT | FE_OVERFLOW) != FE_OVERFLOW) {
        return 4;
    }

    if (fesetround(FE_DOWNWARD) != 0 ||
        mini_test_fegetround() != MINI_FE_DOWNWARD ||
        mini_test_feclearexcept(FE_ALL_EXCEPT) != 0 ||
        mini_test_rint(-1.25) != -2.0 || mini_test_lrint(1.75) != 1L ||
        mini_test_llrintf(-1.25f) != -2LL ||
        (fetestexcept(FE_INEXACT) & FE_INEXACT) == 0) {
        return 5;
    }

    if (mini_test_fesetround(MINI_FE_TOWARDZERO) != 0 ||
        fegetround() != FE_TOWARDZERO || feclearexcept(FE_ALL_EXCEPT) != 0 ||
        mini_test_rint(-1.75) != -1.0 || mini_test_lrintf(1.75f) != 1L ||
        (fetestexcept(FE_INEXACT) & FE_INEXACT) == 0) {
        return 6;
    }

    if (mini_test_fesetround(MINI_FE_TONEAREST) != 0 ||
        feclearexcept(FE_ALL_EXCEPT) != 0 ||
        mini_test_rint(2.5) != 2.0 || mini_test_rint(3.5) != 4.0 ||
        mini_test_llrint(3.5) != 4LL ||
        (fetestexcept(FE_INEXACT) & FE_INEXACT) == 0) {
        return 7;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        mini_test_lrint(9223372036854775808.0) != MINI_LONG_MIN ||
        (fetestexcept(FE_INVALID) & FE_INVALID) == 0) {
        return 8;
    }
    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        mini_test_llrint(quiet_nan()) != MINI_LLONG_MIN ||
        (fetestexcept(FE_INVALID) & FE_INVALID) == 0) {
        return 9;
    }

    if (fesetenv(&saved) != 0) {
        return 10;
    }
    if (puts("fenv-rounding-interop-ok") == EOF) {
        return 11;
    }
    return 0;
}
