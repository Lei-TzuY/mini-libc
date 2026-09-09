# Scaling floating-environment contract

The `scalbn` / `scalbnf` and `ldexp` / `ldexpf` families now participate in
mini-libc's public x86-64 floating environment while preserving the established
bit-level power-of-two scaling algorithm.

## Public result and exception contract

The four public entry points share one binary64 or binary32 scaling core and one
family boundary.

- NaN, infinity, and signed zero pass through unchanged, preserve `errno`, and
  add no exception flag.
- Exact normal results preserve `errno` and add no exception flag.
- Exact subnormal results preserve `errno` and add no exception flag.
- A finite overflow returns the existing signed infinity result, sets
  `errno = ERANGE`, and adds `FE_OVERFLOW | FE_INEXACT`.
- An inexact tiny result, including round-to-zero, retains the existing
  nearest-even subnormal/zero result, sets `errno = ERANGE`, and adds
  `FE_UNDERFLOW | FE_INEXACT`.
- If a tiny mathematical result rounds upward exactly onto the minimum normal
  value, mini-libc does not report underflow after rounding. It preserves
  `errno` and adds `FE_INEXACT` only.
- Incoming sticky flags are additive: the scaling family restores the caller's
  environment before raising any result-class exception.

`ldexp*` and `scalbn*` are aliases at the implementation-contract level; they do
not maintain separate numerical or exception paths.

## Numerical boundary

This phase deliberately does not change the existing scaling result algorithm.
Normal-range scaling edits the encoded exponent. Subnormal creation shifts the
normalized significand and rounds discarded bits to nearest with ties to even.

mini-libc now exposes `fesetround`, but the bit-level tiny-result algorithm still
uses this documented nearest-even rule rather than changing with the active
rounding direction. The present phase is an exception-reporting promotion, not
a claim of rounding-mode-sensitive `scalbn` conformance. A later
`math_errhandling` closure audit must treat that distinction explicitly before
raising the repository-wide conformance claim.

## Archive boundary

Scaling was split out of the older combined decomposition object:

- `src/math/decompose.c` owns `frexp` / `frexpf` and `modf` / `modff` and remains
  independent of `<fenv.h>`;
- `src/math/scale.c` owns `scalbn` / `scalbnf` and `ldexp` / `ldexpf`, including
  the public fenv boundary.

This keeps a program that only needs decomposition from pulling in floating
exception machinery through static-archive unresolved references.

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

The existing numerical differential corpus continues to exercise the exact
scaling results after the archive split.

## Executable evidence

`tests/fenv_scaling_probe.c` is a freestanding static executable. It covers:

- exact normal binary64/binary32 scaling;
- exact minimum subnormal results;
- a half-way tiny result that rounds to minimum normal and raises only
  `FE_INEXACT`;
- positive and negative overflow;
- nonzero and zero inexact underflow;
- float overflow/underflow;
- sticky-flag addition;
- NaN payload and signed-zero pass-through.

`tests/fenv_scaling_interop.c` compiles the production scaling source under
renamed symbols and verifies the same exception flags through the host
`<fenv.h>` ABI while using mini-libc's fenv implementation internally.

Pinned tiny-c compiles `src/math/scale.c` and `tests/tiny_fenv_scaling.c`; both
GNU `ld` and the pinned mini-elf-toolchain link and execute that family contract.
The broader math differential suite remains active after the decomposition /
scaling object split.

## Phase boundary and promotion

`math_errhandling` remains `MATH_ERRNO` in this phase. The scaling family was the
largest remaining public range family whose existing `ERANGE` behavior had not
yet been deliberately reflected into `FE_*` flags.

The next phase is a repository-wide `MATH_ERREXCEPT` closure audit, not another
single-function fenv micro-promotion. It must inventory the remaining exact and
basic math families, explicitly test any signaling-NaN or rounding-mode-sensitive
behavior that can still raise hardware exceptions, preserve the already-promoted
family contracts, and only then consider changing `math_errhandling` to
`MATH_ERRNO | MATH_ERREXCEPT` with GCC, Clang, tiny-c, and mini-elf executable
evidence.
