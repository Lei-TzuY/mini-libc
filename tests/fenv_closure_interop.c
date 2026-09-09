#include <fenv.h>
#include <stdio.h>

extern double mini_test_fabs(double x);
extern double mini_test_copysign(double x, double y);
extern double mini_test_fmin(double x, double y);
extern double mini_test_fmax(double x, double y);
extern double mini_test_trunc(double x);
extern double mini_test_floor(double x);
extern double mini_test_ceil(double x);
extern double mini_test_round(double x);
extern double mini_test_frexp(double x, int *exp);
extern double mini_test_modf(double x, double *iptr);
extern int __mini_fpclassify(double x);

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

int main(void)
{
    double snan = dfrom(0x7ff0000000000001ULL);
    double integral;
    double result;
    int exponent;

    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        feraiseexcept(FE_OVERFLOW) != 0) {
        return 1;
    }
    if (dbits(mini_test_fabs(snan)) != 0x7ff0000000000001ULL ||
        dbits(mini_test_copysign(snan, -0.0)) != 0xfff0000000000001ULL ||
        mini_test_fmin(snan, 2.0) != 2.0 ||
        mini_test_fmax(2.0, snan) != 2.0 ||
        __mini_fpclassify(snan) != 0) {
        return 2;
    }
    exponent = 99;
    result = mini_test_frexp(snan, &exponent);
    if (dbits(result) != 0x7ff0000000000001ULL || exponent != 0) {
        return 3;
    }
    result = mini_test_modf(snan, &integral);
    if (dbits(result) != 0x7ff0000000000001ULL ||
        dbits(integral) != 0x7ff0000000000001ULL ||
        fetestexcept(FE_ALL_EXCEPT) != FE_OVERFLOW) {
        return 4;
    }

    if (feclearexcept(FE_ALL_EXCEPT) != 0 ||
        fesetround(FE_UPWARD) != 0) {
        return 5;
    }
    if (mini_test_fabs(-4.0) != 4.0 || mini_test_trunc(3.0) != 3.0 ||
        mini_test_floor(3.0) != 3.0 || mini_test_ceil(-3.0) != -3.0 ||
        mini_test_round(-3.0) != -3.0) {
        return 6;
    }
    exponent = 99;
    result = mini_test_frexp(8.0, &exponent);
    if (result != 0.5 || exponent != 4) {
        return 7;
    }
    result = mini_test_modf(4.0, &integral);
    if (dbits(result) != 0ULL || integral != 4.0 ||
        fetestexcept(FE_ALL_EXCEPT) != 0) {
        return 8;
    }
    if (fesetround(FE_TONEAREST) != 0) {
        return 9;
    }

    puts("fenv-closure-interop-ok");
    return 0;
}
