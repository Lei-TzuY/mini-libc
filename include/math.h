#ifndef MINI_LIBC_MATH_H
#define MINI_LIBC_MATH_H

#define MATH_ERRNO 1
#define MATH_ERREXCEPT 2
#define math_errhandling (MATH_ERRNO | MATH_ERREXCEPT)

#define FP_NAN 0
#define FP_INFINITE 1
#define FP_ZERO 2
#define FP_SUBNORMAL 3
#define FP_NORMAL 4

int __mini_fpclassify(double x);
int __mini_fpclassifyf(float x);
int __mini_math_predicate(double x, int operation);
int __mini_math_predicatef(float x, int operation);
int __mini_signbit(double x);
int __mini_signbitf(float x);
int __mini_math_compare(double x, double y, int operation);
int __mini_math_comparef(float x, float y, int operation);

#define fpclassify(x) \
    _Generic((x), float: __mini_fpclassifyf, double: __mini_fpclassify)(x)
#define __MINI_MATH_PREDICATE(x, operation) \
    _Generic((x), float: __mini_math_predicatef, \
             double: __mini_math_predicate)((x), (operation))
#define isfinite(x) __MINI_MATH_PREDICATE((x), 0)
#define isinf(x) __MINI_MATH_PREDICATE((x), 1)
#define isnan(x) __MINI_MATH_PREDICATE((x), 2)
#define isnormal(x) __MINI_MATH_PREDICATE((x), 3)
#define signbit(x) \
    _Generic((x), float: __mini_signbitf, double: __mini_signbit)(x)

#define __MINI_MATH_COMPARE(x, y, operation) \
    _Generic(((x) + (y)), float: __mini_math_comparef, \
             double: __mini_math_compare)((x), (y), (operation))
#define isunordered(x, y) __MINI_MATH_COMPARE((x), (y), 0)
#define isgreater(x, y) __MINI_MATH_COMPARE((x), (y), 1)
#define isgreaterequal(x, y) __MINI_MATH_COMPARE((x), (y), 2)
#define isless(x, y) __MINI_MATH_COMPARE((x), (y), 3)
#define islessequal(x, y) __MINI_MATH_COMPARE((x), (y), 4)
#define islessgreater(x, y) __MINI_MATH_COMPARE((x), (y), 5)

double fabs(double x);
float fabsf(float x);
double copysign(double x, double y);
float copysignf(float x, float y);

double fmin(double x, double y);
float fminf(float x, float y);
double fmax(double x, double y);
float fmaxf(float x, float y);

double trunc(double x);
float truncf(float x);
double floor(double x);
float floorf(float x);
double ceil(double x);
float ceilf(float x);
double round(double x);
float roundf(float x);
double rint(double x);
float rintf(float x);
double nearbyint(double x);
float nearbyintf(float x);
long lrint(double x);
long lrintf(float x);
long long llrint(double x);
long long llrintf(float x);

double frexp(double x, int *exp);
float frexpf(float x, int *exp);
double ldexp(double x, int exp);
float ldexpf(float x, int exp);
double scalbn(double x, int n);
float scalbnf(float x, int n);
double modf(double x, double *iptr);
float modff(float x, float *iptr);

double fmod(double x, double y);
float fmodf(float x, float y);
double remainder(double x, double y);
float remainderf(float x, float y);
double remquo(double x, double y, int *quo);
float remquof(float x, float y, int *quo);

double exp(double x);
float expf(float x);
double log(double x);
float logf(float x);
double pow(double x, double y);
float powf(float x, float y);

double sin(double x);
float sinf(float x);
double cos(double x);
float cosf(float x);
double tan(double x);
float tanf(float x);

double atan(double x);
float atanf(float x);
double atan2(double y, double x);
float atan2f(float y, float x);
double asin(double x);
float asinf(float x);
double acos(double x);
float acosf(float x);

double sinh(double x);
float sinhf(float x);
double cosh(double x);
float coshf(float x);
double tanh(double x);
float tanhf(float x);
double asinh(double x);
float asinhf(float x);
double acosh(double x);
float acoshf(float x);
double atanh(double x);
float atanhf(float x);

double erf(double x);
float erff(float x);
double erfc(double x);
float erfcf(float x);

double tgamma(double x);
float tgammaf(float x);
double lgamma(double x);
float lgammaf(float x);

double sqrt(double x);
float sqrtf(float x);

#endif
