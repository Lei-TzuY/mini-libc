#include <errno.h>
#include <math.h>
#include <mini/syscall.h>

#define MINI_PI   3.14159265358979323846264338327950288
#define MINI_PIO2 1.57079632679489661923132169163975144
#define MINI_PIO4 0.785398163397448309615660845819875721

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
    double result;

    if (dbits(atan(-0.0)) != 0x8000000000000000ULL ||
        dbits(atan(0.0)) != 0x0000000000000000ULL ||
        dbits(asin(-0.0)) != 0x8000000000000000ULL) {
        return 1;
    }

    if (!close_double(atan(1.0), MINI_PIO4, 3.0e-16) ||
        !close_double(atan(0.5), 0.46364760900080611621, 4.0e-16) ||
        !close_double(atan(10.0), 1.47112767430373459185, 4.0e-16) ||
        !close_double(atan(-10.0), -1.47112767430373459185, 4.0e-16)) {
        return 2;
    }

    errno = 71;
    if (!close_double(atan(inf), MINI_PIO2, 0.0) || errno != 71 ||
        !close_double(atan(-inf), -MINI_PIO2, 0.0) || errno != 71) {
        return 3;
    }

    if (!close_double(atan2(1.0, 1.0), MINI_PIO4, 4.0e-16) ||
        !close_double(atan2(1.0, -1.0), 3.0 * MINI_PIO4, 5.0e-16) ||
        !close_double(atan2(-1.0, -1.0), -3.0 * MINI_PIO4, 5.0e-16) ||
        !close_double(atan2(-1.0, 1.0), -MINI_PIO4, 4.0e-16)) {
        return 4;
    }

    if (dbits(atan2(-0.0, 1.0)) != 0x8000000000000000ULL ||
        !close_double(atan2(0.0, -1.0), MINI_PI, 0.0) ||
        !close_double(atan2(-0.0, -1.0), -MINI_PI, 0.0) ||
        !close_double(atan2(1.0, 0.0), MINI_PIO2, 0.0) ||
        !close_double(atan2(-1.0, -0.0), -MINI_PIO2, 0.0)) {
        return 5;
    }

    if (!close_double(atan2(inf, inf), MINI_PIO4, 0.0) ||
        !close_double(atan2(inf, -inf), 3.0 * MINI_PIO4, 0.0) ||
        !close_double(atan2(-inf, -inf), -3.0 * MINI_PIO4, 0.0) ||
        !close_double(atan2(1.0, -inf), MINI_PI, 0.0) ||
        dbits(atan2(-1.0, inf)) != 0x8000000000000000ULL) {
        return 6;
    }

    if (!close_double(asin(0.5), 0.52359877559829887308, 5.0e-16) ||
        !close_double(asin(-0.5), -0.52359877559829887308, 5.0e-16) ||
        !close_double(acos(0.5), 1.04719755119659774615, 5.0e-16) ||
        !close_double(acos(-0.5), 2.09439510239319549231, 7.0e-16)) {
        return 7;
    }

    if (!close_double(asin(1.0), MINI_PIO2, 0.0) ||
        !close_double(asin(-1.0), -MINI_PIO2, 0.0) ||
        acos(1.0) != 0.0 || !close_double(acos(-1.0), MINI_PI, 0.0)) {
        return 8;
    }

    errno = 72;
    result = asin(1.0001);
    if (result == result || errno != EDOM) {
        return 9;
    }
    errno = 73;
    result = acos(-1.0001);
    if (result == result || errno != EDOM) {
        return 10;
    }

    errno = 74;
    if (dbits(atan(nan_value)) != 0x7ff8000000000042ULL || errno != 74 ||
        dbits(atan2(nan_value, 1.0)) != 0x7ff8000000000042ULL || errno != 74 ||
        dbits(asin(nan_value)) != 0x7ff8000000000042ULL || errno != 74 ||
        dbits(acos(nan_value)) != 0x7ff8000000000042ULL || errno != 74) {
        return 11;
    }

    errno = 75;
    if (fbits(atanf(fnan)) != 0x7fc01234U || errno != 75 ||
        fbits(asinf(fnan)) != 0x7fc01234U || errno != 75 ||
        fbits(acosf(fnan)) != 0x7fc01234U || errno != 75) {
        return 12;
    }

    if (!close_float(atanf(0.5f), 0.4636476f, 2.0e-7f) ||
        !close_float(atan2f(1.0f, -1.0f), 2.3561945f, 3.0e-7f) ||
        !close_float(asinf(0.5f), 0.5235988f, 2.0e-7f) ||
        !close_float(acosf(0.5f), 1.0471976f, 3.0e-7f)) {
        return 13;
    }

    errno = 76;
    if (!close_double(atan(0.25), 0.24497866312686415417, 4.0e-16) ||
        errno != 76 ||
        !close_double(atan2(3.0, 4.0), 0.64350110879328438680, 5.0e-16) ||
        errno != 76 ||
        !close_double(asin(0.25), 0.25268025514207865349, 5.0e-16) ||
        errno != 76 ||
        !close_double(acos(0.25), 1.31811607165281796575, 7.0e-16) ||
        errno != 76) {
        return 14;
    }

    if (mini_sys_write(1, "inverse-trig-ok\n", 16) != 16) {
        return 15;
    }
    return 0;
}
