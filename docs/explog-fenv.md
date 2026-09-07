# Exception-aware exp/log runtime

## Phase status

The `exp`/`expf` and `log`/`logf` family is the second executable consumer of
mini-libc's x86-64 floating-environment runtime after the rounding family. The
existing errno and numerical approximation contracts remain intact; this phase
adds deliberate sticky `FE_*` propagation for exact, inexact, range, pole, and
domain paths.

This is family-level coverage, not a repository-wide `MATH_ERREXCEPT` claim.
`math_errhandling` therefore remains `MATH_ERRNO` while additional math families
are promoted deliberately.

## Exception contract

`exp` and `expf` use the following policy:

- `exp(+0)` and `exp(-0)` return exactly `1` without changing `errno` or adding
  floating exception flags;
- ordinary nonzero finite results add `FE_INEXACT` while preserving an existing
  `errno` value;
- finite overflow returns positive infinity, sets `errno = ERANGE`, and adds
  `FE_OVERFLOW | FE_INEXACT`;
- finite underflow into the subnormal/zero range sets `errno = ERANGE` and adds
  `FE_UNDERFLOW | FE_INEXACT`;
- infinities and NaNs retain their existing special-value behavior.

The explicit zero fast path is part of the contract rather than only an
optimization. It prevents the general polynomial/range-reduction path from
introducing an artificial `FE_INEXACT` on an exact mathematical result.

`log` and `logf` use the following policy:

- `log(1)` returns exactly zero without changing `errno` or adding exception
  flags;
- ordinary positive finite non-unit results add `FE_INEXACT` while preserving
  an existing `errno` value;
- positive or negative zero returns negative infinity, sets `errno = ERANGE`,
  and adds `FE_DIVBYZERO`;
- a negative nonzero input, including negative infinity, returns a quiet NaN,
  sets `errno = EDOM`, and adds `FE_INVALID`;
- positive infinity and NaNs retain their existing special-value behavior.

The implementation raises public flags through `feraiseexcept`; it does not
invent a second hardware-state path inside the math object. Existing flags are
sticky and remain set across exact calls.

## Numerical boundary

This phase does not change the numerical algorithms, range-reduction constants,
polynomial/series approximations, or public range thresholds already covered by
the `exp`/`log` differential suite. It changes only the floating-environment
side effects associated with already-defined result classes.

No globally correctly-rounded claim is made. The existing controlled numerical
tolerances remain authoritative for ordinary finite values.

## Runtime dependency

Once `exp`/`log` became exception-aware, the floating-environment assembly state
primitive became a real dependency of ordinary math consumers rather than only
of dedicated fenv tests. The pinned tiny-c integration archive therefore now
includes `src/fenv/fenv_asm.S` in its normal assembly member set. This fixes the
actual static-archive dependency graph instead of relying on the later dedicated
fenv integration script to provide the missing symbols.

The normal mini-libc archive already contains `fenv.o`, `fenv_asm.o`, and the math
objects, so ordinary static linking resolves the dependency naturally when an
exception-aware math symbol is pulled in.

## Executable evidence

The freestanding `fenv_explog_probe` verifies:

- exact exception-clean `exp(0)` and `log(1)` paths;
- `FE_INEXACT` for ordinary finite `exp` and `log` results;
- `ERANGE + FE_OVERFLOW + FE_INEXACT` for exponential overflow;
- `ERANGE + FE_UNDERFLOW + FE_INEXACT` for exponential underflow;
- `ERANGE + FE_DIVBYZERO` for logarithm poles;
- `EDOM + FE_INVALID` for logarithm domain failures;
- binary32 overflow, underflow, and invalid-domain wrappers;
- preservation of pre-existing sticky flags across an exact result.

The hosted `fenv_explog_interop` compiles the production exp/log family under
renamed symbols and observes its exception effects through the host `<fenv.h>`.
This proves that mini-libc modifies the actual x87/MXCSR state rather than only a
private software bookkeeping value.

Pinned tiny-c compiles the production decomposition and exp/log objects together
with the fenv-aware integration. The executable verifies overflow, underflow,
pole, domain, and ordinary inexact paths. The same integration is linked and run
with GNU `ld` and the pinned mini-elf-toolchain.

The existing exp/log numerical differential remains in the normal GCC/Clang test
matrix, so adding exception side effects cannot silently change the established
bounded numerical output contract.

## Phase boundary and promotion

The exp/log exception phase is complete. More input values for the same four
functions would now be low-value repetition.

The strongest next family is `pow`/`powf`, because its existing implementation
already exposes multiple errno-classified domain and range paths and composes
through `exp`/`log`. The next slice should make those paths deliberately map to
floating exceptions, including negative-base nonintegral domain failure, zero to
a negative exponent, overflow/underflow, ordinary inexact composition, and exact
integer identities. It must preserve the current result/errno matrix and prove
its `FE_*` effects with freestanding, host-fenv, pinned tiny-c, and mini-elf
execution.

`math_errhandling` must remain `MATH_ERRNO` during that promotion. A global
`MATH_ERREXCEPT` advertisement is justified only after broad family-by-family
executable coverage, not by the presence of the fenv substrate or one or two
exception-aware families.
