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
    double nan_value = dfrom(0x7ff8000000000042ULL);
    double result;
    float fresult;

    if (dbits(sin(-0.0)) != 0x8000000000000000ULL ||
        dbits(tan(-0.0)) != 0x8000000000000000ULL || cos(-0.0) != 1.0) {
        return 1;
    }

    if (!close_double(sin(0.52359877559829887308), 0.5, 2.0e-15) ||
        !close_double(cos(1.04719755119659774615), 0.5, 2.0e-15) ||
        !close_double(tan(0.78539816339744830962), 1.0, 3.0e-15)) {
        return 2;
    }
    if (!close_double(sin(3.14159265358979323846), 0.0, 2.0e-15) ||
        !close_double(cos(3.14159265358979323846), -1.0, 2.0e-15) ||
        !close_double(sin(-1.57079632679489661923), -1.0, 2.0e-15)) {
        return 3;
    }

    if (!close_double(sin(10.0), -0.54402111088936981340, 3.0e-15) ||
        !close_double(cos(1234.5), -0.98937359213242199729, 3.0e-14) ||
        !close_double(tan(1234.5), -0.14695727850342305132, 5.0e-14)) {
        return 4;
    }
    if (!close_double(sin(1000000.0), -0.34999350217129295212, 3.0e-13) ||
        !close_double(cos(1000000.0), 0.93675212753314478694, 3.0e-13) ||
        !close_double(tan(1000000.0), -0.37362445398759902917, 5.0e-13)) {
        return 5;
    }

    if (!close_float(sinf(0.5f), 0.47942555f, 2.0e-7f) ||
        !close_float(cosf(0.5f), 0.87758255f, 2.0e-7f) ||
        !close_float(tanf(0.5f), 0.5463025f, 3.0e-7f)) {
        return 6;
    }

    errno = 71;
    result = sin(0.25);
    if (!(result > 0.0) || errno != 71 || !(cos(0.25) > 0.0) || errno != 71 ||
        !(tan(0.25) > 0.0) || errno != 71) {
        return 7;
    }

    errno = 72;
    if (dbits(sin(nan_value)) != dbits(nan_value) || errno != 72 ||
        dbits(cos(nan_value)) != dbits(nan_value) || errno != 72 ||
        dbits(tan(nan_value)) != dbits(nan_value) || errno != 72) {
        return 8;
    }

    errno = 73;
    result = sin(inf);
    if (result == result || errno != EDOM) {
        return 9;
    }
    errno = 74;
    result = cos(1048577.0);
    if (result == result || errno != EDOM) {
        return 10;
    }
    errno = 75;
    result = tan(-1048577.0);
    if (result == result || errno != EDOM) {
        return 11;
    }

    errno = 76;
    fresult = sinf(2000000.0f);
    if (fresult == fresult || errno != EDOM ||
        (fbits(fresult) & 0x7f800000U) != 0x7f800000U) {
        return 12;
    }

    if (mini_sys_write(1, "trig-ok\n", 8) != 8) {
        return 13;
    }
    return 0;
}
