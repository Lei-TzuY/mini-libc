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

static unsigned int fbits(float value)
{
    union {
        float value;
        unsigned int bits;
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
    double nan_value = from_bits(0x7ff8123456789abcULL);

    if (fegetenv(&saved) != 0 || fesetenv(FE_DFL_ENV) != 0 ||
        feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 1;
    }

    errno = 71;
    if (scalbn(0.75, 4) != 12.0 || ldexp(0.75, 4) != 12.0 ||
        scalbnf(0.75f, 4) != 12.0f || ldexpf(0.75f, 4) != 12.0f ||
        errno != 71 || !only_flags(0)) {
        return 2;
    }

    errno = 72;
    value = scalbn(1.0, -1074);
    fvalue = scalbnf(1.0f, -149);
    if (dbits(value) != 1ULL || fbits(fvalue) != 1U || errno != 72 ||
        !only_flags(0)) {
        return 3;
    }

    if (fesetround(FE_UPWARD) != 0 || feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 24;
    }
    errno = 82;
    value = scalbn(1.0, -1075);
    if (dbits(value) != 1ULL || errno != ERANGE ||
        !only_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 25;
    }
    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 26;
    }
    value = ldexp(-1.0, -1075);
    if (dbits(value) != 0x8000000000000000ULL || errno != ERANGE ||
        !only_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 27;
    }
    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 28;
    }
    fvalue = scalbnf(1.0f, -150);
    if (fbits(fvalue) != 1U || errno != ERANGE ||
        !only_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 29;
    }

    if (fesetround(FE_DOWNWARD) != 0 || feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 30;
    }
    value = scalbn(1.0, -1075);
    if (dbits(value) != 0ULL || errno != ERANGE ||
        !only_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 31;
    }
    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 32;
    }
    value = ldexp(-1.0, -1075);
    if (dbits(value) != 0x8000000000000001ULL || errno != ERANGE ||
        !only_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 33;
    }
    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 34;
    }
    fvalue = scalbnf(-1.0f, -150);
    if (fbits(fvalue) != 0x80000001U || errno != ERANGE ||
        !only_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 35;
    }

    if (fesetround(FE_TOWARDZERO) != 0 ||
        feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 36;
    }
    value = scalbn(1.5, -1074);
    if (dbits(value) != 1ULL || errno != ERANGE ||
        !only_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 37;
    }
    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 38;
    }
    value = scalbn(-1.5, -1074);
    if (dbits(value) != 0x8000000000000001ULL || errno != ERANGE ||
        !only_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 39;
    }

    if (fesetround(FE_TONEAREST) != 0 || feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 40;
    }
    value = scalbn(1.5, -1074);
    if (dbits(value) != 2ULL || errno != ERANGE ||
        !only_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 41;
    }
    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 42;
    }
    value = scalbn(-1.5, -1074);
    if (dbits(value) != 0x8000000000000002ULL || errno != ERANGE ||
        !only_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 43;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 22;
    }
    errno = 81;
    value = scalbn(from_bits(0x3fffffffffffffffULL), -1023);
    if (dbits(value) != 0x0010000000000000ULL || errno != 81 ||
        !only_flags(FE_INEXACT)) {
        return 23;
    }

    if (fesetround(FE_DOWNWARD) != 0 || feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 44;
    }
    errno = 83;
    value = scalbn(from_bits(0x3fffffffffffffffULL), -1023);
    if (dbits(value) != 0x000fffffffffffffULL || errno != ERANGE ||
        !only_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 45;
    }
    if (fesetround(FE_UPWARD) != 0 || feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 46;
    }
    errno = 84;
    value = scalbn(from_bits(0x3fffffffffffffffULL), -1023);
    if (dbits(value) != 0x0010000000000000ULL || errno != 84 ||
        !only_flags(FE_INEXACT)) {
        return 47;
    }
    if (fesetround(FE_TONEAREST) != 0 || feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 48;
    }

    errno = 73;
    value = ldexp(1.0, 1024);
    if (!isinf(value) || signbit(value) || errno != ERANGE ||
        !only_flags(FE_OVERFLOW | FE_INEXACT)) {
        return 5;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 6;
    }
    errno = 74;
    value = scalbn(-1.0, 1024);
    if (!isinf(value) || !signbit(value) || errno != ERANGE ||
        !only_flags(FE_OVERFLOW | FE_INEXACT)) {
        return 7;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 8;
    }
    errno = 75;
    value = scalbn(1.0, -1075);
    if (dbits(value) != 0ULL || errno != ERANGE ||
        !only_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 9;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 10;
    }
    errno = 76;
    value = ldexp(1.5, -1074);
    if (dbits(value) != 2ULL || errno != ERANGE ||
        !only_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 11;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 12;
    }
    errno = 77;
    fvalue = scalbnf(1.0f, -150);
    if (fbits(fvalue) != 0U || errno != ERANGE ||
        !only_flags(FE_UNDERFLOW | FE_INEXACT)) {
        return 13;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 14;
    }
    errno = 78;
    fvalue = ldexpf(1.0f, 128);
    if (!isinf(fvalue) || signbit(fvalue) || errno != ERANGE ||
        !only_flags(FE_OVERFLOW | FE_INEXACT)) {
        return 15;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        feraiseexcept(FE_DIVBYZERO) != 0) {
        return 16;
    }
    errno = 79;
    value = scalbn(1.0, 1024);
    if (!isinf(value) || errno != ERANGE ||
        !only_flags(FE_DIVBYZERO | FE_OVERFLOW | FE_INEXACT)) {
        return 17;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        feraiseexcept(FE_INVALID) != 0) {
        return 18;
    }
    errno = 80;
    if (dbits(scalbn(nan_value, 100)) != 0x7ff8123456789abcULL ||
        dbits(ldexp(-0.0, 100)) != 0x8000000000000000ULL || errno != 80 ||
        !only_flags(FE_INVALID)) {
        return 19;
    }

    if (fesetenv(&saved) != 0) {
        return 20;
    }
    if (mini_sys_write(1, "fenv-scaling-ok\n", 16) != 16) {
        return 21;
    }
    return 0;
}
