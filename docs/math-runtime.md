# Real math runtime ABI and phase status

mini-libc now has an executable `<math.h>` baseline for x86-64 binary32 and
binary64 real operations, plus a shared decomposition/scaling layer that can
serve later transcendental work. The implementation does not link the host libm
and does not use compiler math builtins.

The public surface is:

```c
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

double sqrt(double x);
float sqrtf(float x);
```

`math_errhandling` is `MATH_ERRNO`. `MATH_ERREXCEPT` is defined for source
compatibility, but this phase does not expose `<fenv.h>` or claim floating-point
exception control/reporting.

## Bit-level transforms

`fabs`/`fabsf` and `copysign`/`copysignf` operate on the IEEE-754 sign bit rather
than performing arithmetic. This preserves payload bits for NaNs and handles
signed zero deterministically.

`fmin`/`fmax` and their float variants return the numeric operand when exactly
one input is a NaN. The zero tie is explicit: `fmin(+0,-0)` is `-0` and
`fmax(-0,+0)` is `+0`. No errno change is part of these successful operations.

`trunc`/`truncf` clear fractional significand bits according to the unbiased
binary exponent. `floor`, `ceil`, and `round` build on that integral result.
`round` implements the C rule of halfway cases away from zero. Signed zero is
preserved when the mathematical result is zero, including `ceil(-0.25)`.
Values whose binary exponent already makes them integral are returned without
integer conversion, avoiding integer-range dependencies and signed-overflow
undefined behavior.

## Binary decomposition and scaling

`frexp`/`frexpf` decode the IEEE-754 representation directly. Normal values are
returned with a magnitude in `[0.5, 1)` and a matching integral power-of-two
exponent. Subnormal inputs are explicitly normalized before producing that
fraction, including the minimum binary64 and binary32 subnormals. Zero preserves
its sign and stores exponent zero. Infinity and NaN pass through unchanged;
mini-libc deterministically stores exponent zero for those special inputs.

`scalbn`/`scalbnf` and `ldexp`/`ldexpf` share one bit-level power-of-two scaling
implementation per binary format. Scaling within the normal range only adjusts
the encoded exponent. Scaling into the subnormal range shifts the normalized
significand and rounds discarded bits to nearest with ties to even. The current
runtime has no public rounding-mode control, so this bit-level nearest-even rule
is the explicit mini-libc contract for subnormal creation.

A finite overflow returns a signed infinity and sets `errno = ERANGE`. An
inexact tiny result, including a value rounded to zero, sets `ERANGE` while
returning the nearest-even subnormal/zero result. An exact subnormal result is
not treated as a range failure and leaves an existing errno value unchanged.
Zero, infinity, and NaN pass through unchanged. This distinction is covered by
freestanding and hosted regressions rather than inferred from host errno state.

`modf`/`modff` split a finite value into integral and fractional components
without integer-range conversion. Both components preserve the input sign when
their mathematical value is zero. Infinity stores itself as the integral part
and returns signed zero; NaN is returned and stored as NaN. Successful calls do
not modify errno.

The decomposition/scaling functions live in a separate archive object from the
initial transform/square-root implementation. That keeps the new numerical
layer independently reviewable while exposing one public `<math.h>` surface.

## Square root boundary

`sqrt` and `sqrtf` use dedicated x86-64 SSE helpers (`sqrtsd` and `sqrtss`) for
the actual root operation. Production C performs the public domain policy around
that hardware primitive:

- nonnegative finite values use the hardware result;
- `+inf` remains `+inf`;
- NaNs pass through as NaNs;
- `-0` remains `-0`;
- a negative nonzero input, including `-inf`, returns a quiet NaN and sets
  `errno = EDOM`.

Successful square roots preserve an existing errno value. `EDOM` is part of the
public errno table and maps through `strerror` to
`Numerical argument out of domain`.

The hardware helper is a private implementation detail, not a public ABI. It is
built into the static mini-libc archive and is also explicitly included in the
pinned tiny-c / mini-elf integration archive so the cross-toolchain test does not
silently fall back to host libm.

## Executable evidence

The freestanding `math_probe` is linked only against mini-libc. In addition to
the original sign/minmax/rounding/sqrt checks, it locks minimum-subnormal
`frexp`, subnormal-to-normal scaling, exact-subnormal errno preservation,
inexact nearest-even tiny scaling, tie-to-even underflow to zero, signed overflow
to infinity, and `modf` signed-zero behavior. `make inspect` checks that this
executable has no host CRT/libc/libm dependency.

The hosted `math_differential` compiles both production math objects under
renamed symbols and compares a controlled corpus against the host libm. The
corpus covers normal values, minimum and maximum subnormals, power-of-two scaling
across normal/subnormal boundaries, `frexp` exponents/fractions, `modf` split
results, positive square roots, min/max NaN behavior, and the original rounding
transforms. Mini-libc-specific errno policy is asserted against mini-libc's own
errno storage instead of assuming that host errno behavior defines the contract.

Pinned tiny-c compiles both production C math objects and
`tests/tiny_math_integration.c`. The integration executes decomposition,
normal/subnormal scaling, `modf`, the original transforms, hardware-backed
double/float square roots, and range/domain error paths. Both GNU `ld` and the
pinned mini-elf-toolchain link and run the same executable.

## Phase boundary and promotion

This is still a bounded real-math runtime, not a complete C libm. The current
phase does not expose trigonometric functions, exponential/logarithmic
functions, `pow`, remainder families, classification macros, `<fenv.h>`, complex
math, or long-double variants. No performance or globally correctly-rounded
claim is made beyond the exact hardware `sqrt` result and the explicitly tested
bit-level operations.

The decomposition/scaling phase is now complete, so the next math frontier
should consume it rather than farm more wrappers. The strongest promotion target
is a shared exponential/logarithmic range-reduction layer built on `frexp` and
`scalbn`, followed by executable `exp`/`log` and then `pow` integration. That
phase must define domain/range behavior, use controlled numerical differential
coverage with explicit error tolerances, and continue through pinned tiny-c and
mini-elf execution. A fresh repository-wide audit may still select another
larger standard-runtime subsystem if it provides more integration value at that
checkpoint.
