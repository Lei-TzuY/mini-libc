# Trigonometric runtime ABI and phase status

mini-libc has a bounded executable real trigonometric runtime for x86-64
binary32 and binary64. The public forward-trigonometric surface is:

```c
double sin(double x);
float sinf(float x);
double cos(double x);
float cosf(float x);
double tan(double x);
float tanf(float x);
```

The inverse layer is documented separately in `docs/inverse-trig-runtime.md` and
adds `atan`/`atan2`/`asin`/`acos` plus their binary32 variants.

The implementation is freestanding, links no host libm, and uses no compiler
math builtins. All six forward entry points share one binary64 argument-reduction
layer and one pair of reduced sine/cosine polynomial kernels. The float entry
points reuse that numerical core after preserving binary32 NaN payloads
explicitly. The forward family is now also integrated with the public x86-64
`<fenv.h>` exception environment.

## Bounded range reduction

For finite inputs with `|x| <= 2^20`, mini-libc performs a Cody-Waite style
`pi/2` reduction. It multiplies by `2/pi`, rounds to the nearest integral
quadrant count, and subtracts a split `pi/2` high/low pair. The reduced argument
is kept in the neighborhood of `[-pi/4, pi/4]`. The quadrant count modulo four
then reconstructs the signs and sine/cosine interchange for the original
argument.

This phase deliberately does not claim a Payne-Hanek reducer or globally correct
large-argument trigonometry. A finite input with magnitude greater than `2^20`
returns a quiet NaN and sets `errno = EDOM`. This is a documented bounded
mini-libc extension rather than a claim of complete C/libm conformance. The
boundary exists so numerical error from reducing arbitrarily large binary64
arguments is not hidden behind an unsupported accuracy claim.

## Shared reduced kernels

The reduced sine and cosine kernels are evaluated as odd/even polynomials in
`x^2`. `sin` and `cos` select or negate those two kernel results according to the
quadrant. `tan` deliberately consumes the same reconstructed sine/cosine pair and
returns their quotient instead of growing a third independent range reducer or
polynomial subsystem.

This architecture gives the three forward functions one argument-reduction truth
source and one special-value policy. The inverse-trigonometric layer remains
independent of this large-angle reducer: it owns a separate bounded atan kernel
and quadrant engine rather than coupling inverse functions to the forward
polynomials.

The current implementation is numerically bounded, not correctly rounded.
Controlled host-libm differential tests cover representative positive and
negative values from small arguments through the declared `2^20` range. The
published tolerances are intentionally looser than the observed error and are
part of the test contract rather than a global ULP guarantee. `tan` has a wider
relative tolerance because division amplifies sine/cosine error near poles.

## Public result, errno, and floating-exception behavior

The deterministic mini-libc contract is now:

- `sin(+0)` and `tan(+0)` return `+0` without adding an exception flag;
- `sin(-0)` and `tan(-0)` preserve `-0` without adding an exception flag;
- `cos(+0)` and `cos(-0)` return exactly `1` without adding an exception flag;
- binary64 NaNs are returned unchanged, preserving payload/sign bits and without
  adding a forward-trig-owned severe exception;
- binary32 NaNs passed to `sinf`/`cosf`/`tanf` are returned before widening, so
  their binary32 payload bits are preserved as well;
- positive or negative infinity returns a quiet NaN, sets `EDOM`, and raises
  `FE_INVALID`;
- finite inputs outside the declared reduction bound return a quiet NaN, set
  `EDOM`, and raise `FE_INVALID`;
- supported nonzero finite `sin`, `cos`, and `tan` results preserve the caller's
  incoming `errno` and raise `FE_INEXACT`;
- the binary32 wrappers reuse the same promoted core, so ordinary finite float
  calls observe the same inexact class while retaining the established float
  result/errno contract.

The family never clears caller-owned sticky flags. Exact zero/one and quiet-NaN
paths therefore preserve an already raised exception set. Domain paths add
`FE_INVALID`; ordinary supported finite evaluation adds `FE_INEXACT`.

This remains family-level exception coverage. `math_errhandling` stays
`MATH_ERRNO`; mini-libc does not yet advertise repository-wide `MATH_ERREXCEPT`
until the remaining math families with public errno/range/domain behavior have
corresponding FE_* propagation.

## Executable evidence

The original freestanding `trig_probe` remains the numerical/result checkpoint.
It covers signed-zero behavior, familiar quadrants, medium and large supported
values, binary32 entry points, errno preservation, NaN payload passthrough,
infinity domain failure, and the explicit finite large-argument boundary.

The hosted `trig_differential` still compiles production `trig.c` under renamed
symbols and compares all six functions with the host libm on the same controlled
corpus spanning roughly `[-10^6, 10^6]`. The fenv promotion deliberately leaves
that numerical corpus and tolerances unchanged so exception plumbing cannot hide
or excuse a numerical regression.

The dedicated freestanding `fenv_trig_probe` verifies mini-libc result + errno +
FE_* behavior for:

- exact signed-zero/one paths with a clean environment;
- infinity and the documented finite reduction-bound domain failures;
- ordinary finite `sin`, `cos`, and `tan` inexact evaluation;
- a binary32 `sinf` call through the promoted core; and
- preservation of a pre-existing sticky flag across quiet-NaN and exact paths.

`fenv_trig_interop` compiles the production trig object under renamed symbols and
observes its hardware-visible exception flags through the host `<fenv.h>` ABI.
It validates the x87/MXCSR exception mapping independently of mini-libc's errno
storage.

Pinned tiny-c compiles the production trig source and executes the dedicated
`tiny_fenv_trig` program. The same fenv/trig executable is linked and run through
GNU `ld` and the pinned mini-elf-toolchain, and is included in the
host-libc-independence inspection.

## Phase boundary and promotion

The bounded forward-trigonometric numerical phase is complete, and its established
result/errno classes are now promoted into the public floating environment
without changing the reducer or polynomial kernels. Additional ordinary trig
vectors or wrapper-specific FE_INEXACT cases would be low-value micro-expansion
at this checkpoint.

The remaining high-value family-level floating-exception work should continue by
semantic coverage rather than API count. The strongest current candidates are:

- the `fmod` / `remainder` / `remquo` reduction family, whose zero-divisor and
  infinite-dividend domain results still need public `FE_INVALID` propagation;
- the `erf` / `erfc` family, whose positive-tail range/underflow behavior still
  needs public `FE_UNDERFLOW | FE_INEXACT` propagation and clean-path isolation.

A fresh live-state audit should choose between those families before another
implementation slice is opened. Long-double math, complex arithmetic,
locale-sensitive numerical behavior, a full large-argument Payne-Hanek reducer,
and globally correctly-rounded transcendental guarantees remain separate phases.
