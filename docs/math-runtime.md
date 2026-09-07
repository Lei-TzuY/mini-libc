# Basic real math runtime ABI and phase status

mini-libc now has a bounded executable `<math.h>` baseline for x86-64 binary32
and binary64 real operations. This phase establishes a real libm-style runtime
surface without linking the host libm and without using compiler math builtins.

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

Successful square roots preserve an existing errno value. `EDOM` is now part of
the public errno table and maps through `strerror` to
`Numerical argument out of domain`.

The hardware helper is a private implementation detail, not a public ABI. It is
built into the static mini-libc archive and is also explicitly included in the
pinned tiny-c / mini-elf integration archive so the cross-toolchain test does not
silently fall back to host libm.

## Executable evidence

The freestanding `math_probe` is linked only against mini-libc and locks exact
binary representations for sign operations, signed-zero min/max behavior,
rounding behavior, and the known binary64/binary32 encodings of `sqrt(2)`. It
also proves errno preservation on successful sqrt and `EDOM` on negative input.
`make inspect` checks that this executable has no host CRT/libc/libm dependency.

The hosted `math_differential` compiles the production math implementation under
renamed symbols and compares a controlled corpus against the host libm. The
corpus covers positive/negative fractions, signed zeros, values near the exact
integer precision boundaries, NaN min/max behavior, sign copying, and positive
square roots. Domain-error checks inspect mini-libc's own errno storage rather
than the host errno object.

Pinned tiny-c compiles the production C implementation and
`tests/tiny_math_integration.c`. The integration executes sign transforms,
min/max, rounding functions, hardware-backed double/float square roots, and the
negative-sqrt `EDOM` path. Both GNU `ld` and the pinned mini-elf-toolchain link
and run the same executable, including the hand-written SSE square-root object.

## Phase boundary and promotion

This is a basic real-math runtime, not a complete C libm. The current phase does
not expose trigonometric functions, exponential/logarithmic functions, `pow`,
remainder/decomposition families, classification macros, `<fenv.h>`, complex
math, or long-double variants. No performance or correctly-rounded claim is made
beyond the exact hardware `sqrt` result and the explicitly tested bit-level
operations.

The next math phase should add a coherent numerical capability rather than farm
single wrappers. A strong promotion target is a shared binary decomposition and
scaling layer (`frexp`/`ldexp`/`scalbn`/`modf`) that can become infrastructure
for later `exp`/`log`/`pow` work, with controlled differential and pinned
cross-toolchain evidence. A fresh repository-wide audit may still choose another
larger standard-runtime gap (for example locale infrastructure) if it has higher
integration value when this phase closes.
