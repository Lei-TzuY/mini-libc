#include <fenv.h>
#include <stdio.h>

typedef unsigned int mini_fexcept_t;

typedef struct {
    unsigned short __x87_control;
    unsigned short __x87_exceptions;
    unsigned int __mxcsr;
} mini_fenv_t;

int mini_test_feclearexcept(int excepts);
int mini_test_fegetexceptflag(mini_fexcept_t *flagp, int excepts);
int mini_test_feraiseexcept(int excepts);
int mini_test_fesetexceptflag(const mini_fexcept_t *flagp, int excepts);
int mini_test_fetestexcept(int excepts);
int mini_test_fegetround(void);
int mini_test_fesetround(int round);
int mini_test_fegetenv(mini_fenv_t *envp);
int mini_test_feholdexcept(mini_fenv_t *envp);
int mini_test_fesetenv(const mini_fenv_t *envp);
int mini_test_feupdateenv(const mini_fenv_t *envp);

#define MINI_FE_INVALID 0x01
#define MINI_FE_DIVBYZERO 0x04
#define MINI_FE_OVERFLOW 0x08
#define MINI_FE_UNDERFLOW 0x10
#define MINI_FE_INEXACT 0x20
#define MINI_FE_ALL_EXCEPT 0x3d
#define MINI_FE_TONEAREST 0x0000
#define MINI_FE_DOWNWARD 0x0400
#define MINI_FE_UPWARD 0x0800
#define MINI_FE_TOWARDZERO 0x0c00

int main(void)
{
    fenv_t saved;
    mini_fenv_t mini_saved;
    mini_fexcept_t flags;

    if (FE_INVALID != MINI_FE_INVALID ||
        FE_DIVBYZERO != MINI_FE_DIVBYZERO ||
        FE_OVERFLOW != MINI_FE_OVERFLOW ||
        FE_UNDERFLOW != MINI_FE_UNDERFLOW ||
        FE_INEXACT != MINI_FE_INEXACT ||
        FE_TONEAREST != MINI_FE_TONEAREST ||
        FE_DOWNWARD != MINI_FE_DOWNWARD ||
        FE_UPWARD != MINI_FE_UPWARD ||
        FE_TOWARDZERO != MINI_FE_TOWARDZERO) {
        return 1;
    }
    if (fegetenv(&saved) != 0 || fesetenv(FE_DFL_ENV) != 0) {
        return 2;
    }

    if (mini_test_fesetround(MINI_FE_UPWARD) != 0 ||
        fegetround() != FE_UPWARD) {
        return 3;
    }
    if (fesetround(FE_DOWNWARD) != 0 ||
        mini_test_fegetround() != MINI_FE_DOWNWARD) {
        return 4;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        mini_test_feraiseexcept(MINI_FE_INVALID | MINI_FE_INEXACT) != 0 ||
        fetestexcept(FE_INVALID | FE_INEXACT) !=
            (FE_INVALID | FE_INEXACT)) {
        return 5;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        feraiseexcept(FE_OVERFLOW | FE_UNDERFLOW) != 0 ||
        mini_test_fetestexcept(MINI_FE_OVERFLOW | MINI_FE_UNDERFLOW) !=
            (MINI_FE_OVERFLOW | MINI_FE_UNDERFLOW)) {
        return 6;
    }

    if (mini_test_fegetexceptflag(&flags, MINI_FE_ALL_EXCEPT) != 0 ||
        flags != (MINI_FE_OVERFLOW | MINI_FE_UNDERFLOW)) {
        return 7;
    }
    if (mini_test_feclearexcept(MINI_FE_ALL_EXCEPT) != 0 ||
        fetestexcept(FE_ALL_EXCEPT) != 0) {
        return 8;
    }
    if (mini_test_fesetexceptflag(&flags, MINI_FE_ALL_EXCEPT) != 0 ||
        fetestexcept(FE_OVERFLOW | FE_UNDERFLOW) !=
            (FE_OVERFLOW | FE_UNDERFLOW)) {
        return 9;
    }

    if (mini_test_fegetenv(&mini_saved) != 0 ||
        fesetround(FE_TOWARDZERO) != 0 ||
        feclearexcept(FE_ALL_EXCEPT) != 0 ||
        mini_test_fesetenv(&mini_saved) != 0 ||
        fegetround() != FE_DOWNWARD ||
        fetestexcept(FE_OVERFLOW | FE_UNDERFLOW) !=
            (FE_OVERFLOW | FE_UNDERFLOW)) {
        return 10;
    }

    if (fesetenv(&saved) != 0) {
        return 11;
    }
    if (puts("fenv-interop-ok") == EOF) {
        return 12;
    }
    return 0;
}
