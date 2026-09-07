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
    float fvalue;

    errno = 71;
    if (!close_double(erf(0.5), 0.52049987781304653768, 2.0e-15) ||
        !close_double(erfc(0.5), 0.47950012218695346232, 2.0e-15) ||
        errno != 71) {
        return 1;
    }

    errno = 72;
    if (!close_double(erf(1.5), 0.96610514647531072707, 2.0e-15) ||
        !close_double(erf(-1.5), -0.96610514647531072707, 2.0e-15) ||
        errno != 72) {
        return 2;
    }

    errno = 73;
    if (!close_double(erfc(4.0), 1.5417257900280018852e-8, 2.0e-21) ||
        !close_double(erfc(10.0), 2.0884875837625447570e-45, 3.0e-58) ||
        errno != 73) {
        return 3;
    }

    if (erf(inf) != 1.0 || erf(-inf) != -1.0 ||
        erfc(inf) != 0.0 || erfc(-inf) != 2.0) {
        return 4;
    }
    if (dbits(erf(nan_value)) != dbits(nan_value) ||
        dbits(erfc(nan_value)) != dbits(nan_value) ||
        fbits(erff(fnan)) != fbits(fnan) || fbits(erfcf(fnan)) != fbits(fnan)) {
        return 5;
    }
    if (dbits(erf(dfrom(0x8000000000000000ULL))) != 0x8000000000000000ULL) {
        return 6;
    }

    errno = 74;
    value = erfc(30.0);
    if (value != 0.0 || errno != ERANGE) {
        return 7;
    }
    errno = 75;
    value = erfc(-30.0);
    if (value != 2.0 || errno != 75) {
        return 8;
    }

    errno = 76;
    fvalue = erff(0.5f);
    if (!close_float(fvalue, 0.5204999f, 2.0e-7f) ||
        !close_float(erfcf(2.0f), 0.004677735f, 2.0e-8f) || errno != 76) {
        return 9;
    }
    errno = 77;
    fvalue = erfcf(12.0f);
    if (fvalue != 0.0f || errno != ERANGE) {
        return 10;
    }

    if (mini_sys_write(1, "special-ok\n", 11) != 11) {
        return 11;
    }
    return 0;
}
