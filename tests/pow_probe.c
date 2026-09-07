#include <errno.h>
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

static unsigned int fbits(float value)
{
    union {
        float value;
        unsigned int bits;
    } convert;

    convert.value = value;
    return convert.bits;
}

static int close_double(double actual, double expected, double tolerance)
{
    double difference = actual - expected;

    if (difference < 0.0) {
        difference = -difference;
    }
    return difference <= tolerance;
}

static int close_float(float actual, float expected, float tolerance)
{
    float difference = actual - expected;

    if (difference < 0.0f) {
        difference = -difference;
    }
    return difference <= tolerance;
}

int main(void)
{
    double inf = dfrom(0x7ff0000000000000ULL);
    double neg_inf = dfrom(0xfff0000000000000ULL);
    double qnan = dfrom(0x7ff8000000000042ULL);
    double neg_zero = dfrom(0x8000000000000000ULL);
    double result;
    float fresult;

    if (pow(2.0, 10.0) != 1024.0 || pow(2.0, -3.0) != 0.125 ||
        pow(-2.0, 3.0) != -8.0 || pow(-2.0, 4.0) != 16.0 ||
        pow(2.5, 3.0) != 15.625) {
        return 1;
    }
    if (!close_double(pow(9.0, 0.5), 3.0, 2.0e-12) ||
        !close_double(pow(5.0, 1.25), 7.476743906106102, 4.0e-12)) {
        return 2;
    }

    errno = 71;
    result = pow(1.25, 4.0);
    if (result != 2.44140625 || errno != 71) {
        return 3;
    }
    errno = 72;
    result = pow(-2.0, 0.5);
    if (result == result || errno != EDOM) {
        return 4;
    }

    if (pow(qnan, 0.0) != 1.0 || pow(1.0, qnan) != 1.0 ||
        pow(-1.0, inf) != 1.0 || dbits(pow(qnan, 2.0)) != dbits(qnan)) {
        return 5;
    }
    if (dbits(pow(neg_zero, 3.0)) != dbits(neg_zero) ||
        dbits(pow(neg_zero, 2.0)) != 0ULL) {
        return 6;
    }
    errno = 73;
    result = pow(0.0, -2.0);
    if (dbits(result) != dbits(inf) || errno != ERANGE) {
        return 7;
    }
    errno = 74;
    result = pow(neg_zero, -3.0);
    if (dbits(result) != dbits(neg_inf) || errno != ERANGE) {
        return 8;
    }

    if (dbits(pow(inf, -2.0)) != 0ULL ||
        dbits(pow(neg_inf, 3.0)) != dbits(neg_inf) ||
        dbits(pow(neg_inf, 2.0)) != dbits(inf) ||
        dbits(pow(0.5, inf)) != 0ULL || dbits(pow(2.0, neg_inf)) != 0ULL ||
        dbits(pow(2.0, inf)) != dbits(inf)) {
        return 9;
    }

    errno = 75;
    result = pow(10.0, 400.0);
    if (dbits(result) != dbits(inf) || errno != ERANGE) {
        return 10;
    }
    errno = 76;
    result = pow(10.0, -400.0);
    if (dbits(result) != 0ULL || errno != ERANGE) {
        return 11;
    }

    if (powf(-2.0f, 3.0f) != -8.0f ||
        !close_float(powf(9.0f, 0.5f), 3.0f, 3.0e-6f)) {
        return 12;
    }
    errno = 77;
    fresult = powf(10.0f, 100.0f);
    if ((fbits(fresult) & 0x7f800000U) != 0x7f800000U || errno != ERANGE) {
        return 13;
    }
    errno = 78;
    fresult = powf(-2.0f, 0.5f);
    if (fresult == fresult || errno != EDOM) {
        return 14;
    }

    if (mini_sys_write(1, "pow-ok\n", 7) != 7) {
        return 15;
    }
    return 0;
}
