# Error-function runtime

## Phase status

This phase is complete for the current binary32/binary64 mini-libc math target.
The public `<math.h>` surface now includes `erf`, `erfc`, `erff`, and `erfcf`
with executable freestanding, controlled differential, pinned tiny-c, and
mini-elf evidence.

This checkpoint is a bounded special-function implementation, not a claim of
complete C math conformance, correctly-rounded results for every binary input,
long-double support, or floating-environment exception support.

## Numerical structure

`src/math/special.c` implements the error-function family with a piecewise
rational approximation:

- `|x| <= 1` uses a central `erf` rational;
- `1 <= x < 8` uses a direct complementary-error rational;
- `x >= 8` uses a separate `erfc` tail rational.

The positive tail is computed directly instead of forming `1 - erf(x)`. This
preserves useful relative accuracy when the complementary probability is much
smaller than one, including the regression points around `erfc(4)` and
`erfc(10)`.

The tail exponential uses the existing mini-libc `exp` implementation. The
special-function object saves/restores the caller's incoming `errno` around
that internal dependency, so successful finite calls do not leak an internal
range status.

## Public behavior

The current deterministic special-value and errno policy is:

- NaN payloads are propagated unchanged;
- `erf(+inf) = +1` and `erf(-inf) = -1`;
- `erfc(+inf) = 0` and `erfc(-inf) = 2`;
- `erf(-0)` preserves the negative-zero sign;
- successful ordinary finite results preserve incoming `errno`;
- positive finite `erfc` tails that narrow into the subnormal/zero range set
  `errno = ERANGE`;
- negative tails approach two without reporting the positive-tail underflow as
  a user-visible error;
- `erfcf` independently checks binary32 narrowing and reports positive-tail
  underflow with `ERANGE`.

No floating exception flags are claimed because `<fenv.h>` is outside the
current runtime boundary and `math_errhandling` remains `MATH_ERRNO`.

## Archive boundary

The implementation is an independent `math_special.o` archive object. It
reuses the established exponential/decomposition substrate but has no
dependency on trig, inverse trig, hyperbolic, power, or remainder objects.
Programs that only need the error-function family therefore do not pull the
rest of the newer math layers into the static link.

## Executable evidence

`tests/special_probe.c` is a freestanding static executable covering:

- central and transition-region values;
- symmetry of `erf`;
- cancellation-sensitive complementary tails;
- very small positive tails;
- NaN payload propagation;
- infinities and signed zero;
- positive-tail underflow and negative-tail errno isolation;
- binary32 variants and narrowing behavior.

`tests/special_differential.c` compiles the production special-function object
under renamed symbols and compares it with the host libm over controlled
binary64 and binary32 corpora. The differential links mini-libc's own renamed
`exp` and decomposition objects, so host libm does not provide the production
exponential dependency.

Pinned tiny-c compiles the production special-function source as part of the
complete mini-libc source set and executes representative `erf`/`erfc`, float
variants, and range paths in `tests/tiny_math_integration.c`. The same
integration executable is linked and run through GNU `ld` and the pinned
mini-elf-toolchain.

## Next frontier

The error-function checkpoint established the first dedicated special-function
layer. The following Gamma promotion now lives beside it as an independent
archive member with shared Lanczos/reflection semantics; see
`docs/gamma-runtime.md` for that current checkpoint.

With both error-function and Gamma families executable, the next architectural
gap is no longer another isolated special-function wrapper. The strongest
promotion is a floating-environment foundation (`<fenv.h>`) that exposes
rounding-mode and exception-status state coherently across x86-64 SSE/MXCSR and
x87, enabling later rounding-sensitive math behavior to be verified honestly.

Long-double special functions, complex arithmetic, and globally
correctly-rounded transcendental guarantees remain separate phases.
