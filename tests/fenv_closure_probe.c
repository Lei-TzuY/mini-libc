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

static double dfrom(unsigned long long bits)
{
    union {
        double value;
        unsigned long long bits;
    } convert;

    convert.bits = bits;
    return convert.value;
}

static int flags_equal(int expected)
{
    return fetestexcept(FE_ALL_EXCEPT) == expected;
}

int main(void)
{
    double snan = dfrom(0x7ff0000000000001ULL);
    double integral;
    double result;
    int exponent;

    if (math_errhandling != (MATH_ERRNO | MATH_ERREXCEPT)) {
        return 1;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        feraiseexcept(FE_OVERFLOW) != 0) {
        return 2;
    }
    errno = 71;
    if (dbits(fabs(snan)) != 0x7ff0000000000001ULL ||
        dbits(copysign(snan, -0.0)) != 0xfff0000000000001ULL ||
        fmin(snan, 2.0) != 2.0 || fmax(2.0, snan) != 2.0 ||
        fpclassify(snan) != FP_NAN || !isnan(snan)) {
        return 3;
    }
    exponent = 99;
    result = frexp(snan, &exponent);
    if (dbits(result) != 0x7ff0000000000001ULL || exponent != 0) {
        return 4;
    }
    result = modf(snan, &integral);
    if (dbits(result) != 0x7ff0000000000001ULL ||
        dbits(integral) != 0x7ff0000000000001ULL || errno != 71 ||
        !flags_equal(FE_OVERFLOW)) {
        return 5;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        fesetround(FE_UPWARD) != 0) {
        return 6;
    }
    errno = 72;
    if (fabs(-4.0) != 4.0 || trunc(3.0) != 3.0 ||
        floor(3.0) != 3.0 || ceil(-3.0) != -3.0 || round(-3.0) != -3.0) {
        return 7;
    }
    exponent = 99;
    result = frexp(8.0, &exponent);
    if (result != 0.5 || exponent != 4) {
        return 8;
    }
    result = modf(4.0, &integral);
    if (dbits(result) != 0ULL || integral != 4.0 || errno != 72 ||
        !flags_equal(0)) {
        return 9;
    }
    if (fesetround(FE_TONEAREST) != 0) {
        return 10;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 11;
    }
    errno = 73;
    result = sqrt(-1.0);
    if (result == result || errno != EDOM || !flags_equal(FE_INVALID)) {
        return 12;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 13;
    }
    errno = 74;
    result = log(0.0);
    if (!(result < -1.0e308) || errno != ERANGE ||
        !flags_equal(FE_DIVBYZERO)) {
        return 14;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 15;
    }
    errno = 75;
    result = exp(1000.0);
    if (!(result > 1.0e308) || errno != ERANGE ||
        !flags_equal(FE_OVERFLOW | FE_INEXACT)) {
        return 16;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0) {
        return 17;
    }
    errno = 76;
    result = scalbn(1.0, -1075);
    if (dbits(result) != 0ULL || errno != ERANGE ||
        !flags_equal(FE_UNDERFLOW | FE_INEXACT)) {
        return 18;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        feraiseexcept(FE_INVALID) != 0) {
        return 19;
    }
    errno = 77;
    if (fabs(-2.0) != 2.0 || copysign(2.0, -1.0) != -2.0 ||
        errno != 77 || !flags_equal(FE_INVALID)) {
        return 20;
    }

    if (mini_sys_write(1, "fenv-closure-ok\n", 16) != 16) {
        return 21;
    }
    return 0;
}
