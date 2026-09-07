# Hyperbolic runtime ABI and phase status

mini-libc now has a bounded executable real-hyperbolic layer for x86-64
binary32 and binary64. The public surface is:

```c
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
```

The implementation is freestanding, links no host libm, and uses no compiler
math builtins. It deliberately reuses the existing `exp`, `log`, and `sqrt`
runtimes instead of growing an independent transcendental substrate.

## Forward hyperbolic layer

For small magnitudes, `sinh` and `cosh` use bounded power-series kernels so
subtractive cancellation in `(exp(x) - exp(-x)) / 2` does not destroy small
results. `tanh` reuses those two small kernels.

For ordinary magnitudes, `sinh`/`cosh` use the existing exponential runtime.
For large magnitudes they evaluate `exp(|x| - ln(2))` instead of forming
`exp(|x|) / 2`. This preserves a finite binary64 result beyond the direct
`exp` overflow threshold and reaches the actual bounded `sinh`/`cosh` overflow
boundary near `710.47586`. A finite overflow returns signed infinity for
`sinh`, positive infinity for `cosh`, and sets `errno = ERANGE`.

`tanh` never needs a large positive exponential. Magnitudes above the bounded
saturation threshold return exactly `+1` or `-1`, avoiding an intermediate
underflow/overflow errno side effect. Signed zero is preserved.

## Inverse hyperbolic composition

`asinh`, `acosh`, and `atanh` reuse the existing logarithm and square-root
substrate.

- small `asinh` and `atanh` arguments use convergent odd series to avoid
  cancellation;
- ordinary `asinh(x)` uses a stable
  `log(1 + |x| + x^2 / (1 + sqrt(1+x^2)))` form;
- ordinary `acosh(x)` uses `log(x + sqrt((x-1)(x+1)))`;
- very large `asinh`/`acosh` inputs use `log(|x|) + ln(2)`, avoiding overflow
  in an intermediate square;
- `atanh(x)` uses the logarithmic ratio only after the small-argument region.

`acosh(x)` for finite `x < 1` returns a quiet NaN and sets `EDOM`.
`atanh(x)` for `|x| > 1` returns a quiet NaN and sets `EDOM`; exactly `+/-1`
returns signed infinity and sets `ERANGE`.

NaNs pass through without changing errno. `sinh`, `tanh`, `asinh`, and `atanh`
preserve signed zero. Infinities follow the real mathematical limits, including
`cosh(-inf) = +inf` and `tanh(+/-inf) = +/-1`.

## Binary32 behavior

The six `*f` entry points preserve binary32 NaNs before widening. Finite values
reuse the binary64 core and narrow the result. `sinhf`/`coshf` explicitly detect
a finite binary32 input whose narrowed result becomes infinity and set
`ERANGE`; the remaining binary32 functions inherit the domain/pole behavior of
the shared binary64 core.

This phase does not add long-double, complex-math, floating-environment, or
alternate-rounding-mode contracts.

## Executable evidence

The freestanding `hyperbolic_probe` checks:

- signed-zero behavior and exact special-value limits;
- representative forward and inverse values;
- a finite `sinh(710)` result beyond the direct `exp(710)` range;
- binary64 `sinh`/`cosh` overflow and `ERANGE`;
- large-argument `tanh` saturation without errno pollution;
- large `asinh` without square overflow;
- `acosh`/`atanh` domain and pole errors;
- NaN payload passthrough;
- binary32 entry points and binary32 narrowing overflow.

The hosted `hyperbolic_differential` compiles production `hyperbolic.c` under
renamed symbols and compares all twelve public functions with host libm across
small, medium, large, endpoint-oriented, and binary32 inputs. Explicit
relative/absolute tolerances are the numerical contract; no globally
correctly-rounded or ULP bound is claimed.

Pinned tiny-c compiles `hyperbolic.c` with the rest of mini-libc and executes
ordinary and binary32 hyperbolic calls through `tiny_math_integration`. Both GNU
`ld` and the pinned mini-elf-toolchain link and run the same executable, so this
phase is covered by the existing three-repo gate rather than only host
GCC/Clang builds.

## Phase boundary and promotion

The bounded real transcendental runtime now includes exponential/logarithmic,
power, forward trigonometric, inverse trigonometric, and forward/inverse
hyperbolic families. Remaining major `<math.h>` gaps include the remainder
family, public classification/comparison macros, error/gamma functions,
long-double families, `<fenv.h>`, and complex math.

The next strong architectural promotion is a coherent remainder and floating
classification subsystem rather than more hyperbolic aliases or wrapper-only
functions. A useful vertical slice should establish bit-level public
classification/comparison primitives together with `fmod`/`remainder`/`remquo`
and their binary32 variants, including huge-quotient reduction, signed-zero,
NaN/infinity/domain semantics, controlled host differential coverage, and the
pinned tiny-c/mini-elf executable gate. A fresh live audit should still recheck
repository state before implementation.
