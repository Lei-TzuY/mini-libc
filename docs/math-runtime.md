# Real math runtime ABI and phase status

mini-libc now has an executable `<math.h>` baseline for x86-64 binary32 and
binary64 real operations, a shared decomposition/scaling layer, and a bounded
exponential/logarithmic runtime built directly on that layer. The implementation
does not link the host libm and does not use compiler math builtins.

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

double exp(double x);
float expf(float x);
double log(double x);
float logf(float x);

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
initial transform/square-root implementation. That keeps the numerical layer
independently reviewable while exposing one public `<math.h>` surface.

## Exponential and logarithmic range reduction

`exp` consumes the existing power-of-two scaling layer instead of growing an
independent exponent-construction path. The implementation multiplies by
`1/ln(2)`, rounds to the nearest integral binary exponent, and subtracts a split
`ln(2)` high/low pair so the reduced argument remains close to zero. A degree-13
polynomial then approximates `exp(r)` on that bounded interval and `scalbn`
restores the selected binary exponent.

Successful normal results preserve an existing errno value. A finite overflow
returns positive infinity with `ERANGE`. A finite result in the binary64
subnormal range, including underflow to zero, is treated as a range result and
sets `ERANGE`; this is intentionally stricter than the exact-subnormal policy of
raw `scalbn`, because mathematical `exp(x)` is not represented by an exact
power-of-two scale of its reduced polynomial in the general case. `exp(+inf)` is
`+inf`, `exp(-inf)` is positive zero, and NaNs pass through unchanged.

`expf` uses the binary64 range-reduction core and then narrows to binary32. A
finite narrowed infinity, subnormal, or zero is a binary32 range result and sets
`ERANGE`. Float NaNs and infinities are handled before widening so their public
special-value behavior is deterministic.

`log` starts with `frexp`, then conditionally doubles the mantissa so it lies near
one. The reduced logarithm uses
`2 * (y + y^3/3 + y^5/5 + ...)`, where `y = (m - 1) / (m + 1)`, through the
`1/25` term. The final binary exponent is combined using the same split `ln(2)`
constants used by `exp`. Positive finite inputs and `+inf` preserve an existing
errno value. Zero returns negative infinity with `ERANGE`; a negative nonzero
input, including `-inf`, returns a quiet NaN with `EDOM`; NaNs pass through.
`logf` widens an exactly representable binary32 input through the same core and
narrows the result.

These are bounded numerical implementations, not correctly-rounded libm claims.
The implementation deliberately exposes its approximation contract through
controlled host-libm differential tests with explicit relative/absolute
tolerances. The current polynomial/series choices are intended to provide useful
near-machine-precision binary64 behavior across the tested range, but no global
ULP guarantee is claimed.

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

The freestanding `math_probe` is linked only against mini-libc. It locks the
bit-level transform, decomposition/scaling, and hardware square-root contracts,
including minimum-subnormal `frexp`, normal/subnormal scaling boundaries,
nearest-even tiny scaling, signed overflow to infinity, and `modf` signed-zero
behavior.

The freestanding `explog_probe` independently locks known `exp`/`log` values,
normal errno preservation, infinity/NaN behavior, binary64 overflow and
underflow, logarithm domain/range errors, minimum-subnormal logarithms, and the
binary32 narrowing range contract. Both freestanding probes are checked by
`make inspect` for host CRT/libc/libm independence.

The hosted `math_differential` continues to compare the exact/bit-oriented math
objects against a controlled host-libm corpus. The separate
`explog_differential` compiles production `explog.c` and `decompose.c` under
renamed symbols, then compares `exp`/`log` across wide normal dynamic ranges and
the float variants across representative binary32 inputs. The exponential checks
use explicit relative/absolute tolerances, and logarithm checks use an explicit
absolute floor plus relative tolerance; this is numerical evidence, not a
bit-exact conformance claim. Mini-libc-specific errno paths are asserted against
mini-libc's own errno storage.

Pinned tiny-c compiles every production math C object and
`tests/tiny_math_integration.c`. The integration now executes `exp`, `log`,
`expf`, and `logf` alongside decomposition/scaling, transforms, hardware-backed
square roots, and range/domain error paths. Both GNU `ld` and the pinned
mini-elf-toolchain link and run the same executable.

## Phase boundary and promotion

This is still a bounded real-math runtime, not a complete C libm. The current
phase does not expose `pow`, trigonometric functions, remainder families,
classification macros, `<fenv.h>`, complex math, or long-double variants. No
performance or globally correctly-rounded claim is made beyond the exact
hardware `sqrt` result, the explicitly tested bit-level operations, and the
stated numerical tolerances for `exp`/`log`.

The exponential/logarithmic phase is now complete, so the next math frontier
should consume it rather than farm more wrappers. The strongest promotion target
is `pow`/`powf`: it should combine exact integer-exponent fast paths with the
shared `exp(y * log(x))` machinery for positive noninteger bases, while defining
negative-base integer detection, zero/infinity/NaN matrices, domain/range errno
behavior, and controlled numerical tolerances. That slice must continue through
freestanding probes, host differential coverage, pinned tiny-c, and mini-elf
execution. A fresh repository-wide audit may still select another larger
standard-runtime subsystem if it provides more integration value at that
checkpoint.
