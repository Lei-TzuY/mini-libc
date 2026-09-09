#include <errno.h>
#include <fenv.h>
#include <math.h>
#include <mini/syscall.h>

static unsigned long long dbits(double value)
{
    union {
        double value;
        unsigned long long bits;
    } convert;

    convert.value = value;
    return convert.bits;
}

static double from_bits(unsigned long long bits)
{
    union {
        double value;
        unsigned long long bits;
    } convert;

    convert.bits = bits;
    return convert.value;
}

static int only_flags(int flags)
{
    return fetestexcept(FE_ALL_EXCEPT) == flags;
}

int main(void)
{
    fenv_t saved;
    double value;
    float fvalue;
    double inf = from_bits(0x7ff0000000000000ULL);
    double nan_value = from_bits(0x7ff8123456789abcULL);

    if (fegetenv(&saved) != 0 || fesetenv(FE_DFL_ENV) != 0 ||
        feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 1;
    }

    errno = 73;
    value = erf(-0.0);
    if (dbits(value) != 0x8000000000000000ULL || erfc(0.0) != 1.0 ||
        erf(inf) != 1.0 || erf(-inf) != -1.0 || erfc(inf) != 0.0 ||
        erfc(-inf) != 2.0 || errno != 73 || !only_flags(0)) {
        return 2;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 3;
    }
    errno = 74;
    value = erf(0.5);
    if (!(value > 0.52 && value < 0.521) || errno != 74 ||
        !only_flags(FE_INEXACT)) {
        return 4;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 5;
    }
    errno = 75;
    value = erfc(0.5);
    if (!(value > 0.479 && value < 0.480) || errno != 75 ||
        !only_flags(FE_INEXACT)) {
        return 6;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 7;
    }
    errno = 76;
    value = erf(8.0);
    if (value != 1.0 || errno != 76 || !only_flags(FE_INEXACT)) {
        return 8;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 9;
    }
    errno = 77;
    value = erfc(30.0);
    if (value != 0.0 || errno != ERANGE ||
        !only_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 10;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 11;
    }
    errno = 78;
    value = erfc(-30.0);
    if (value != 2.0 || errno != 78 || !only_flags(FE_INEXACT)) {
        return 12;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 13;
    }
    errno = 79;
    fvalue = erff(0.5f);
    if (!((double)fvalue > 0.52 && (double)fvalue < 0.521) || errno != 79 ||
        !only_flags(FE_INEXACT)) {
        return 14;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 15;
    }
    errno = 80;
    fvalue = erfcf(12.0f);
    if (fvalue != 0.0f || errno != ERANGE ||
        !only_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 16;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        feraiseexcept(FE_DIVBYZERO) != 0) {
        return 17;
    }
    errno = 81;
    value = erfc(-30.0);
    if (value != 2.0 || errno != 81 ||
        !only_flags(FE_DIVBYZERO | FE_INEXACT)) {
        return 18;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        feraiseexcept(FE_OVERFLOW) != 0) {
        return 19;
    }
    errno = 82;
    if (dbits(erf(nan_value)) != 0x7ff8123456789abcULL ||
        erf(0.0) != 0.0 || erfc(-inf) != 2.0 || errno != 82 ||
        !only_flags(FE_OVERFLOW)) {
        return 20;
    }

    if (fesetenv(&saved) != 0) {
        return 21;
    }
    if (mini_sys_write(1, "fenv-special-ok\n", 16) != 16) {
        return 22;
    }
    return 0;
}
