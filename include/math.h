#ifndef MINI_LIBC_MATH_H
#define MINI_LIBC_MATH_H

#define MATH_ERRNO 1
#define MATH_ERREXCEPT 2
#define math_errhandling MATH_ERRNO

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

double frexp(double x, int *exp);
float frexpf(float x, int *exp);
double ldexp(double x, int exp);
float ldexpf(float x, int exp);
double scalbn(double x, int n);
float scalbnf(float x, int n);
double modf(double x, double *iptr);
float modff(float x, float *iptr);

double exp(double x);
float expf(float x);
double log(double x);
float logf(float x);
double pow(double x, double y);
float powf(float x, float y);

double sqrt(double x);
float sqrtf(float x);

#endif
