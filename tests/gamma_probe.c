#include <errno.h>
#include <math.h>
#include <mini/syscall.h>

static double dfrom(unsigned long long bits)
{
    union {
        double value;
        unsigned long long bits;
    } convert;

    convert.bits = bits;
    return convert.value;
}

static unsigned long long dbits(double value)
{
    union {
        double value;
        unsigned long long bits;
    } convert;

    convert.value = value;
    return convert.bits;
}

static float ffrom(unsigned int bits)
{
    union {
        float value;
        unsigned int bits;
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

static int close_double(double actual, double expected, double rel, double abs)
{
    double difference = actual - expected;
    double magnitude = expected;

    if (difference < 0.0) {
        difference = -difference;
    }
    if (magnitude < 0.0) {
        magnitude = -magnitude;
    }
    return difference <= abs || difference <= rel * magnitude;
}

static int close_float(float actual, float expected, float rel, float abs)
{
    float difference = actual - expected;
    float magnitude = expected;

    if (difference < 0.0f) {
        difference = -difference;
    }
    if (magnitude < 0.0f) {
        magnitude = -magnitude;
    }
    return difference <= abs || difference <= rel * magnitude;
}

int main(void)
{
    double inf = dfrom(0x7ff0000000000000ULL);
    double nan_value = dfrom(0x7ff8000000000042ULL);
    float fnan = ffrom(0x7fc01234U);
    double value;
    float fvalue;

    errno = 101;
    if (!close_double(tgamma(0.5), 1.77245385090551602730, 2.0e-14, 1.0e-15) ||
        !close_double(tgamma(5.0), 24.0, 2.0e-14, 1.0e-14) ||
        !close_double(lgamma(5.0), 3.17805383034794561965, 2.0e-14, 1.0e-14) ||
        errno != 101) {
        return 1;
    }

    errno = 102;
    if (!close_double(tgamma(-0.5), -3.54490770181103205460, 2.0e-14, 1.0e-14) ||
        !close_double(tgamma(-1.5), 2.36327180120735470306, 2.0e-14, 1.0e-14) ||
        !close_double(lgamma(-0.5), 1.26551212348464539649, 2.0e-14, 1.0e-14) ||
        errno != 102) {
        return 2;
    }

    errno = 103;
    value = tgamma(172.0);
    if ((dbits(value) & 0x7fffffffffffffffULL) != 0x7ff0000000000000ULL ||
        errno != ERANGE) {
        return 3;
    }

    errno = 104;
    value = tgamma(-180.5);
    if (value != 0.0 || (dbits(value) & 0x8000000000000000ULL) == 0ULL ||
        errno != ERANGE) {
        return 4;
    }

    errno = 105;
    value = tgamma(-2.0);
    if (value == value || errno != EDOM) {
        return 5;
    }

    errno = 106;
    value = lgamma(-2.0);
    if (dbits(value) != 0x7ff0000000000000ULL || errno != ERANGE) {
        return 6;
    }

    errno = 107;
    value = tgamma(0.0);
    if (dbits(value) != 0x7ff0000000000000ULL || errno != ERANGE) {
        return 7;
    }
    errno = 108;
    value = tgamma(dfrom(0x8000000000000000ULL));
    if (dbits(value) != 0xfff0000000000000ULL || errno != ERANGE) {
        return 8;
    }

    errno = 109;
    if (tgamma(inf) != inf || lgamma(inf) != inf || lgamma(-inf) != inf ||
        errno != 109) {
        return 9;
    }
    errno = 110;
    value = tgamma(-inf);
    if (value == value || errno != EDOM) {
        return 10;
    }

    errno = 111;
    if (dbits(tgamma(nan_value)) != dbits(nan_value) ||
        dbits(lgamma(nan_value)) != dbits(nan_value) ||
        fbits(tgammaf(fnan)) != fbits(fnan) ||
        fbits(lgammaf(fnan)) != fbits(fnan) || errno != 111) {
        return 11;
    }

    errno = 112;
    fvalue = tgammaf(5.0f);
    if (!close_float(fvalue, 24.0f, 3.0e-6f, 1.0e-6f) ||
        !close_float(lgammaf(-0.5f), 1.2655121f, 3.0e-6f, 2.0e-6f) ||
        errno != 112) {
        return 12;
    }
    errno = 113;
    fvalue = tgammaf(36.0f);
    if ((fbits(fvalue) & 0x7fffffffU) != 0x7f800000U || errno != ERANGE) {
        return 13;
    }

    if (mini_sys_write(1, "gamma-ok\n", 9) != 9) {
        return 14;
    }
    return 0;
}
