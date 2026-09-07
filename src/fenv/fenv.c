#include <fenv.h>

#define MINI_X87_EXCEPTION_MASK 0x003fU
#define MINI_MXCSR_EXCEPTION_MASK 0x0000003fU
#define MINI_MXCSR_MASK_BITS 0x00001f80U
#define MINI_X87_ROUND_MASK 0x0c00U
#define MINI_MXCSR_ROUND_MASK 0x00006000U

extern void __mini_fenv_read(fenv_t *env);
extern void __mini_fenv_write(const fenv_t *env);

const fenv_t __mini_fe_dfl_env = {0x037fU, 0U, 0x00001f80U};

static unsigned int public_mask(int excepts)
{
    return (unsigned int)excepts & FE_ALL_EXCEPT;
}

int fegetenv(fenv_t *envp)
{
    if (envp == 0) {
        return -1;
    }
    __mini_fenv_read(envp);
    return 0;
}

int fesetenv(const fenv_t *envp)
{
    if (envp == 0) {
        return -1;
    }
    __mini_fenv_write(envp);
    return 0;
}

int fetestexcept(int excepts)
{
    fenv_t env;
    unsigned int mask = public_mask(excepts);

    __mini_fenv_read(&env);
    return (int)(((unsigned int)env.__x87_exceptions | env.__mxcsr) & mask);
}

int feclearexcept(int excepts)
{
    fenv_t env;
    unsigned int mask = public_mask(excepts);

    __mini_fenv_read(&env);
    env.__x87_exceptions =
        (unsigned short)(env.__x87_exceptions & (unsigned short)~mask);
    env.__mxcsr &= ~mask;
    __mini_fenv_write(&env);
    return 0;
}

int feraiseexcept(int excepts)
{
    fenv_t env;
    unsigned int mask = public_mask(excepts);

    __mini_fenv_read(&env);
    env.__x87_exceptions =
        (unsigned short)(env.__x87_exceptions | (unsigned short)mask);
    env.__mxcsr |= mask;
    __mini_fenv_write(&env);
    return 0;
}

int fegetexceptflag(fexcept_t *flagp, int excepts)
{
    if (flagp == 0) {
        return -1;
    }
    *flagp = (fexcept_t)fetestexcept(excepts);
    return 0;
}

int fesetexceptflag(const fexcept_t *flagp, int excepts)
{
    fenv_t env;
    unsigned int mask;
    unsigned int requested;

    if (flagp == 0) {
        return -1;
    }
    mask = public_mask(excepts);
    requested = *flagp & mask;
    __mini_fenv_read(&env);
    env.__x87_exceptions = (unsigned short)(
        ((unsigned int)env.__x87_exceptions & ~mask) | requested);
    env.__mxcsr = (env.__mxcsr & ~mask) | requested;
    __mini_fenv_write(&env);
    return 0;
}

int fegetround(void)
{
    fenv_t env;
    unsigned int x87;
    unsigned int sse;

    __mini_fenv_read(&env);
    x87 = env.__x87_control & MINI_X87_ROUND_MASK;
    sse = (env.__mxcsr & MINI_MXCSR_ROUND_MASK) >> 3;
    return x87 == sse ? (int)x87 : -1;
}

int fesetround(int round)
{
    fenv_t env;
    unsigned int mode = (unsigned int)round;

    if (mode != FE_TONEAREST && mode != FE_DOWNWARD && mode != FE_UPWARD &&
        mode != FE_TOWARDZERO) {
        return -1;
    }

    __mini_fenv_read(&env);
    env.__x87_control = (unsigned short)(
        ((unsigned int)env.__x87_control & ~MINI_X87_ROUND_MASK) | mode);
    env.__mxcsr =
        (env.__mxcsr & ~MINI_MXCSR_ROUND_MASK) | (mode << 3);
    __mini_fenv_write(&env);
    return 0;
}

int feholdexcept(fenv_t *envp)
{
    fenv_t held;

    if (envp == 0) {
        return -1;
    }
    __mini_fenv_read(envp);
    held.__x87_control =
        (unsigned short)(envp->__x87_control | MINI_X87_EXCEPTION_MASK);
    held.__x87_exceptions = 0U;
    held.__mxcsr =
        (envp->__mxcsr | MINI_MXCSR_MASK_BITS) & ~MINI_MXCSR_EXCEPTION_MASK;
    __mini_fenv_write(&held);
    return 0;
}

int feupdateenv(const fenv_t *envp)
{
    int raised;

    if (envp == 0) {
        return -1;
    }
    raised = fetestexcept(FE_ALL_EXCEPT);
    if (fesetenv(envp) != 0) {
        return -1;
    }
    return feraiseexcept(raised);
}
