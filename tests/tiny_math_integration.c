#include <errno.h>
#include <math.h>
#include <stdio.h>

int main(void)
{
    double bad;

    if (fabs(-3.5) != 3.5 || fabsf(-2.25f) != 2.25f) {
        return 1;
    }
    if (copysign(2.0, -1.0) != -2.0 ||
        copysignf(2.0f, -1.0f) != -2.0f) {
        return 2;
    }
    if (fmin(-0.0, 0.0) != 0.0 || fmax(-0.0, 0.0) != 0.0) {
        return 3;
    }
    if (trunc(-1.75) != -1.0 || floor(-1.25) != -2.0 ||
        ceil(1.25) != 2.0 || round(2.5) != 3.0 || round(-2.5) != -3.0) {
        return 4;
    }
    if (sqrt(81.0) != 9.0 || sqrtf(16.0f) != 4.0f) {
        return 5;
    }

    errno = 77;
    bad = sqrt(-1.0);
    if (bad == bad || errno != EDOM) {
        return 6;
    }

    if (puts("tiny-math-ok") == EOF) {
        return 7;
    }
    return 0;
}
