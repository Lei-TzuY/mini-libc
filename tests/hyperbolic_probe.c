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

static float ffrom(unsigned int bits)
{
    union {
        float value;
        unsigned int bits;
    } convert;

    convert.bits = bits;
    return convert.value;
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
    double nan_value = dfrom(0x7ff8000000000042ULL);
    float fnan = ffrom(0x7fc01234U);
    double value;

    if (dbits(sinh(-0.0)) != 0x8000000000000000ULL ||
        dbits(tanh(-0.0)) != 0x8000000000000000ULL ||
        dbits(asinh(-0.0)) != 0x8000000000000000ULL ||
        dbits(atanh(-0.0)) != 0x8000000000000000ULL ||
        cosh(-0.0) != 1.0 || acosh(1.0) != 0.0) {
        return 1;
    }

    errno = 51;
    if (!close_double(sinh(1.0), 1.17520119364380145688, 4.0e-14) ||
        !close_double(cosh(1.0), 1.54308063481524377848, 4.0e-14) ||
        !close_double(tanh(1.0), 0.76159415595576488812, 4.0e-14) ||
        errno != 51) {
        return 2;
    }

    if (!close_double(asinh(1.0), 0.88137358701954302523, 5.0e-14) ||
        !close_double(acosh(2.0), 1.31695789692481670863, 5.0e-14) ||
        !close_double(atanh(0.5), 0.54930614433405484570, 5.0e-14) ||
        errno != 51) {
        return 3;
    }

    errno = 52;
    value = sinh(710.0);
    if (!(value > 1.0e307) || !(value < inf) || errno != 52) {
        return 4;
    }
    errno = 53;
    value = sinh(711.0);
    if (dbits(value) != 0x7ff0000000000000ULL || errno != ERANGE) {
        return 5;
    }
    errno = 54;
    value = cosh(-711.0);
    if (dbits(value) != 0x7ff0000000000000ULL || errno != ERANGE) {
        return 6;
    }

    errno = 55;
    if (tanh(1000.0) != 1.0 || tanh(-1000.0) != -1.0 || errno != 55) {
        return 7;
    }
    if (!close_double(asinh(1.0e200), 461.210165779369082, 2.0e-12) ||
        errno != 55) {
        return 8;
    }

    errno = 56;
    value = acosh(0.5);
    if (value == value || errno != EDOM) {
        return 9;
    }
    errno = 57;
    value = atanh(1.25);
    if (value == value || errno != EDOM) {
        return 10;
    }
    errno = 58;
    value = atanh(-1.0);
    if (dbits(value) != 0xfff0000000000000ULL || errno != ERANGE) {
        return 11;
    }

    errno = 59;
    if (dbits(sinh(inf)) != 0x7ff0000000000000ULL ||
        dbits(sinh(-inf)) != 0xfff0000000000000ULL ||
        dbits(cosh(-inf)) != 0x7ff0000000000000ULL ||
        tanh(inf) != 1.0 || tanh(-inf) != -1.0 ||
        dbits(asinh(-inf)) != 0xfff0000000000000ULL ||
        dbits(acosh(inf)) != 0x7ff0000000000000ULL || errno != 59) {
        return 12;
    }

    errno = 60;
    if (dbits(sinh(nan_value)) != 0x7ff8000000000042ULL ||
        dbits(cosh(nan_value)) != 0x7ff8000000000042ULL ||
        dbits(tanh(nan_value)) != 0x7ff8000000000042ULL ||
        dbits(asinh(nan_value)) != 0x7ff8000000000042ULL ||
        dbits(acosh(nan_value)) != 0x7ff8000000000042ULL ||
        dbits(atanh(nan_value)) != 0x7ff8000000000042ULL || errno != 60) {
        return 13;
    }

    if (!close_float(sinhf(1.0f), 1.1752012f, 3.0e-6f) ||
        !close_float(coshf(1.0f), 1.5430807f, 3.0e-6f) ||
        !close_float(tanhf(1.0f), 0.7615942f, 3.0e-6f) ||
        !close_float(asinhf(1.0f), 0.8813736f, 3.0e-6f) ||
        !close_float(acoshf(2.0f), 1.3169579f, 3.0e-6f) ||
        !close_float(atanhf(0.5f), 0.54930615f, 3.0e-6f)) {
        return 14;
    }

    errno = 61;
    value = (double)sinhf(100.0f);
    if (fbits((float)value) != 0x7f800000U || errno != ERANGE) {
        return 15;
    }
    errno = 62;
    if (fbits(sinhf(fnan)) != 0x7fc01234U ||
        fbits(acoshf(fnan)) != 0x7fc01234U || errno != 62) {
        return 16;
    }

    if (mini_sys_write(1, "hyperbolic-ok\n", 14) != 14) {
        return 17;
    }
    return 0;
}
