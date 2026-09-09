# Scaling floating-environment contract

The `scalbn` / `scalbnf` and `ldexp` / `ldexpf` families participate in
mini-libc's public x86-64 floating environment while preserving one shared
bit-level power-of-two scaling implementation per binary format.

## Public result and exception contract

The four public entry points share one binary64 or binary32 scaling core and one
family boundary.

- NaN, infinity, and signed zero pass through unchanged, preserve `errno`, and
  add no exception flag.
- Exact normal results preserve `errno` and add no exception flag.
- Exact subnormal results preserve `errno` and add no exception flag under every
  public rounding mode.
- A finite overflow keeps the established signed-infinity result, sets
  `errno = ERANGE`, and adds `FE_OVERFLOW | FE_INEXACT`.
- An inexact tiny result is rounded according to the active `fegetround()` mode.
  If the rounded result remains subnormal or zero, mini-libc sets
  `errno = ERANGE` and adds `FE_UNDERFLOW | FE_INEXACT`.
- If an inexact tiny mathematical result rounds onto the minimum normal value,
  mini-libc applies tininess-after-rounding: it preserves `errno` and adds only
  `FE_INEXACT`.
- Incoming sticky flags are additive: the scaling family restores the caller's
  environment before raising any result-class exception.

`ldexp*` and `scalbn*` are aliases at the implementation-contract level; they do
not maintain separate numerical or exception paths.

## Directed tiny-result rounding

Subnormal creation is a pure integer shift/round operation over the normalized
significand. Discarded bits are interpreted through the active public rounding
mode instead of an unconditional nearest-even rule:

- `FE_TONEAREST` rounds to nearest with ties to even;
- `FE_TOWARDZERO` truncates discarded magnitude;
- `FE_UPWARD` increments an inexact positive magnitude and truncates an inexact
  negative magnitude;
- `FE_DOWNWARD` increments an inexact negative magnitude and truncates an
  inexact positive magnitude.

This rule also covers shifts that discard the entire significand. For example,
half of the minimum positive subnormal rounds to the minimum subnormal under
`FE_UPWARD`, but to positive zero under `FE_DOWNWARD`, `FE_TOWARDZERO`, and the
ties-to-even `FE_TONEAREST` case. The negative half-way result is symmetric:
`FE_DOWNWARD` produces the negative minimum subnormal while `FE_UPWARD` and
`FE_TOWARDZERO` produce negative zero.

The minimum-normal boundary is classified after rounding. A value immediately
below the boundary rounds to the minimum normal under `FE_TONEAREST` or positive
`FE_UPWARD` and therefore raises only `FE_INEXACT`; under positive
`FE_DOWNWARD`/`FE_TOWARDZERO` it remains the maximum subnormal and reports
`ERANGE + FE_UNDERFLOW | FE_INEXACT`.

The active rounding mode does not affect exact normal/subnormal results because
no discarded bit exists on those paths.

## Current overflow boundary

This promotion is deliberately limited to tiny-result creation, the numerical
gap identified by the repository-wide `MATH_ERREXCEPT` closure. Finite overflow
still returns signed infinity under every rounding mode. The phase therefore
does not claim complete IEC 60559 directed-overflow behavior for `scalbn*` /
`ldexp*`; overflow endpoint selection remains a separate rounding-mode audit.

## Archive boundary

Scaling remains split from the older combined decomposition object:

- `src/math/decompose.c` owns `frexp` / `frexpf` and `modf` / `modff` and remains
  independent of `<fenv.h>`;
- `src/math/scale.c` owns `scalbn` / `scalbnf` and `ldexp` / `ldexpf`, including
  active-rounding tiny-result creation and the public fenv boundary.

This keeps a program that only needs decomposition from pulling in floating
environment machinery through static-archive unresolved references.

`mk/fenv-scale.mk` owns the scaling-specific production object, renamed hosted
object, freestanding probe, host-fenv interop executable, and the additional
link dependencies required by existing math differentials that reach scaling
through `exp`, `pow`, remainder reduction, hyperbolic, gamma, or error-function
composition.

## Internal composition

Some higher math families use `scalbn` internally. Their public contracts remain
stable:

- remainder-family finite reduction already snapshots and restores the caller's
  environment, so implementation scaling cannot leak flags through `fmod`,
  `remainder`, or `remquo`;
- `exp` deliberately maps its final finite result class to public overflow,
  underflow, or inexact behavior, so scaling range flags agree with that family
  boundary rather than weakening it.

The existing numerical differential corpus continues to exercise exact scaling
results after the archive split and rounding-mode promotion.

## Executable evidence

`tests/fenv_scaling_probe.c` is a freestanding static executable. It covers:

- exact normal and minimum-subnormal binary64/binary32 scaling;
- positive and negative half-min-subnormal cases under directed rounding;
- `1.5 * minimum-subnormal` nearest versus truncating results;
- the minimum-normal boundary with `FE_INEXACT` versus
  `FE_UNDERFLOW | FE_INEXACT` selected by the rounded result class;
- positive and negative overflow;
- ordinary inexact underflow and round-to-zero;
- float directed half-way behavior;
- sticky-flag addition;
- NaN payload and signed-zero pass-through.

`tests/fenv_scaling_interop.c` compiles the production scaling source under
renamed symbols, lets the host `<fenv.h>` select all four public rounding modes,
and verifies the same result bits and exception flags through the host fenv ABI.

Pinned tiny-c compiles `src/math/scale.c` and `tests/tiny_fenv_scaling.c`; the
integration executes directed binary64/binary32 tiny-result cases. Both GNU `ld`
and the pinned mini-elf-toolchain link and execute that family contract. The
broader math differential suite remains active after the decomposition/scaling
object split.

## Phase boundary and promotion

Active-rounding subnormal creation is now part of the executable scaling
baseline rather than a documented exception to it. Repeating additional half-way
vectors would be low-value corner-case farming.

The remaining scaling rounding-mode gap is finite-overflow endpoint selection:
this phase intentionally preserves the established signed-infinity result even
under directed modes. The next repository-wide rounding-mode audit should treat
that gap together with other bounded math kernels that still hard-code
nearest-even or otherwise ignore the active direction, rather than opening a
single-function wrapper-only follow-up. Any promotion must preserve current
errno/FE_* contracts and continue through freestanding, host-fenv, pinned tiny-c,
and mini-elf executable evidence.
