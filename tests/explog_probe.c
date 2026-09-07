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
    double nan_value = dfrom(0x7ff8000000000042ULL);
    double result;
    float fresult;

    if (exp(0.0) != 1.0 || log(1.0) != 0.0) {
        return 1;
    }
    if (!close_double(exp(0.69314718055994530942), 2.0, 2.0e-15) ||
        !close_double(log(2.0), 0.69314718055994530942, 2.0e-15)) {
        return 2;
    }
    if (!close_double(exp(1.0), 2.71828182845904523536, 3.0e-15)) {
        return 3;
    }
    result = exp(1.25);
    if (!close_double(log(result), 1.25, 2.0e-13)) {
        return 4;
    }
    if (!close_float(expf(1.0f), 2.7182817f, 2.0e-6f) ||
        !close_float(logf(2.0f), 0.6931472f, 2.0e-6f)) {
        return 5;
    }

    errno = 71;
    result = exp(0.25);
    if (!(result > 1.0) || errno != 71) {
        return 6;
    }
    result = log(4.0);
    if (!(result > 1.0) || errno != 71) {
        return 7;
    }

    if (dbits(exp(inf)) != dbits(inf) || dbits(exp(neg_inf)) != 0ULL ||
        dbits(exp(nan_value)) != dbits(nan_value)) {
        return 8;
    }
    errno = 72;
    result = exp(1000.0);
    if (dbits(result) != dbits(inf) || errno != ERANGE) {
        return 9;
    }
    errno = 73;
    result = exp(-1000.0);
    if (dbits(result) != 0ULL || errno != ERANGE) {
        return 10;
    }
    errno = 74;
    result = exp(-744.0);
    if (result == 0.0 || errno != ERANGE) {
        return 11;
    }

    errno = 75;
    if (dbits(log(inf)) != dbits(inf) || errno != 75 ||
        dbits(log(nan_value)) != dbits(nan_value) || errno != 75) {
        return 12;
    }
    errno = 76;
    result = log(0.0);
    if (dbits(result) != dbits(neg_inf) || errno != ERANGE) {
        return 13;
    }
    errno = 77;
    result = log(-1.0);
    if (result == result || errno != EDOM) {
        return 14;
    }
    errno = 78;
    result = log(dfrom(1ULL));
    if (!close_double(result, -744.44007192138121809, 2.0e-12) || errno != 78) {
        return 15;
    }

    errno = 79;
    fresult = expf(100.0f);
    if ((fbits(fresult) & 0x7f800000U) != 0x7f800000U || errno != ERANGE) {
        return 16;
    }
    errno = 80;
    fresult = logf(-1.0f);
    if (fresult == fresult || errno != EDOM) {
        return 17;
    }

    if (mini_sys_write(1, "explog-ok\n", 10) != 10) {
        return 18;
    }
    return 0;
}
