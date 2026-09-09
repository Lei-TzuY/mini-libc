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
    int quotient;
    double inf = from_bits(0x7ff0000000000000ULL);
    double nan_value = from_bits(0x7ff8123456789abcULL);

    if (fegetenv(&saved) != 0 || fesetenv(FE_DFL_ENV) != 0 ||
        feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 1;
    }

    errno = 71;
    quotient = 99;
    if (fmod(13.0, 4.0) != 1.0 || remainder(6.0, 4.0) != -2.0 ||
        remquo(30.0, 4.0, &quotient) != -2.0 || quotient != 8 ||
        errno != 71 || !only_flags(0)) {
        return 2;
    }

    errno = 72;
    quotient = 99;
    if (fmodf(13.0f, 4.0f) != 1.0f ||
        remainderf(6.0f, 4.0f) != -2.0f ||
        remquof(-30.0f, -4.0f, &quotient) != 2.0f || quotient != 8 ||
        errno != 72 || !only_flags(0)) {
        return 3;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        feraiseexcept(FE_DIVBYZERO) != 0) {
        return 4;
    }
    errno = 73;
    value = fmod(5.3, 2.0);
    if (!(value > 1.29 && value < 1.31) || errno != 73 ||
        !only_flags(FE_DIVBYZERO)) {
        return 5;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 6;
    }
    errno = 74;
    value = fmod(1.0, 0.0);
    if (!isnan(value) || errno != EDOM || !only_flags(FE_INVALID)) {
        return 7;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 8;
    }
    errno = 75;
    value = remainder(inf, 2.0);
    if (!isnan(value) || errno != EDOM || !only_flags(FE_INVALID)) {
        return 9;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 10;
    }
    errno = 76;
    quotient = 99;
    value = remquo(1.0, 0.0, &quotient);
    if (!isnan(value) || quotient != 0 || errno != EDOM ||
        !only_flags(FE_INVALID)) {
        return 11;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 12;
    }
    errno = 77;
    fvalue = remainderf(1.0f, 0.0f);
    if (!isnan(fvalue) || errno != EDOM || !only_flags(FE_INVALID)) {
        return 13;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        feraiseexcept(FE_OVERFLOW) != 0) {
        return 14;
    }
    errno = 78;
    quotient = 99;
    fvalue = remquof(1.0f, 0.0f, &quotient);
    if (!isnan(fvalue) || quotient != 0 || errno != EDOM ||
        !only_flags(FE_OVERFLOW | FE_INVALID)) {
        return 15;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        feraiseexcept(FE_UNDERFLOW) != 0) {
        return 16;
    }
    errno = 79;
    if (dbits(fmod(nan_value, 2.0)) != 0x7ff8123456789abcULL ||
        fmod(3.0, inf) != 3.0 || dbits(remainder(-0.0, 3.0)) !=
            0x8000000000000000ULL ||
        errno != 79 || !only_flags(FE_UNDERFLOW)) {
        return 17;
    }

    if (fesetenv(&saved) != 0) {
        return 18;
    }
    if (mini_sys_write(1, "fenv-remainder-ok\n", 18) != 18) {
        return 19;
    }
    return 0;
}
