# Error-function runtime

## Phase status

This phase is complete for the current binary32/binary64 mini-libc math target.
The public `<math.h>` surface includes `erf`, `erfc`, `erff`, and `erfcf` with
freestanding, controlled differential, host-fenv interoperability, pinned
tiny-c, and mini-elf executable evidence.

The numerical approximation is unchanged by the floating-environment promotion.
This remains a bounded special-function implementation, not a claim of complete
C math conformance, correctly-rounded results for every binary input,
long-double support, or repository-wide `MATH_ERREXCEPT` coverage.

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

The tail exponential uses mini-libc's existing `exp` implementation. Public
error-function entry points snapshot the caller's x87/MXCSR environment before
nontrivial approximation work, execute all internal rational/exponential
arithmetic, restore that snapshot, and then deliberately raise only the
exception class owned by the public result. This prevents implementation-detail
flags from internal `exp`, multiplication, or division from leaking through the
family boundary while preserving caller-owned sticky flags and rounding state.

## Public errno and floating-exception behavior

The deterministic family contract is:

- quiet NaN payloads are propagated unchanged without adding an error-function
  exception flag;
- `erf(+inf) = +1`, `erf(-inf) = -1`, `erfc(+inf) = 0`, and `erfc(-inf) = 2`
  without adding a new flag;
- `erf(-0)` preserves the negative-zero sign and zero/infinity exact paths
  preserve incoming `errno` and sticky flags;
- ordinary finite nonzero `erf` / `erfc` evaluation preserves incoming `errno`
  and raises `FE_INEXACT`;
- positive finite `erfc` results in the binary64 subnormal/zero range retain the
  existing `errno = ERANGE` behavior and raise `FE_UNDERFLOW | FE_INEXACT`;
- negative finite `erfc` approaches two with `FE_INEXACT` only: an underflow in
  the hidden positive-tail computation is deliberately isolated and is not a
  caller-visible `FE_UNDERFLOW`;
- `erff` reclassifies its final binary32 result and raises `FE_INEXACT` for
  ordinary finite nonzero evaluation;
- positive finite `erfcf` results that narrow into the binary32 subnormal/zero
  range set `ERANGE` and raise `FE_UNDERFLOW | FE_INEXACT`;
- all deliberate flags are additive: a caller's pre-existing sticky exception
  set is restored before the family adds its own result-class flag.

This is family-level exception coverage. `math_errhandling` remains
`MATH_ERRNO`; mini-libc does not advertise repository-wide `MATH_ERREXCEPT`
until the remaining math families with public domain/range behavior have been
promoted as well.

## Archive boundary

The implementation remains the independent `math_special.o` archive object. It
reuses the established exponential/decomposition and fenv substrate but has no
dependency on trig, inverse trig, hyperbolic, power, or remainder objects.
Programs that only need the error-function family therefore do not pull those
unrelated math layers into the static link.

## Executable evidence

The original `tests/special_probe.c` and `tests/special_differential.c` remain the
numerical/result checkpoint. Their numerical corpora and tolerances are unchanged
by the floating-environment promotion. The differential links mini-libc's
renamed fenv runtime as well as its renamed `exp`/decomposition dependency,
avoiding an invalid cross-ABI mix between mini-libc's `fenv_t` and the host libc
fenv functions.

`tests/fenv_special_probe.c` is a freestanding static executable covering:

- exact zero/infinity paths with a clean environment;
- ordinary finite `erf` and `erfc` inexact signaling;
- finite `erf` saturation to one as an inexact mathematical result;
- positive binary64 `erfc` underflow;
- negative-tail isolation proving hidden positive-tail underflow is not exposed;
- binary32 `erff` and `erfcf` outward classification; and
- preservation/combination of caller-owned sticky flags across exact and
  nontrivial paths.

`tests/fenv_special_interop.c` compiles the production special-function object
under renamed symbols and observes its hardware-visible flags through the host
`<fenv.h>` ABI while using mini-libc's fenv implementation internally.

Pinned tiny-c compiles the promoted production `special.c` together with the
fenv, decomposition, and exp/log runtime, then executes `tiny_fenv_special`.
The same executable is linked and run through GNU `ld` and the pinned
mini-elf-toolchain and is included in host-libc-independence inspection.

## Phase boundary and promotion

The error-function numerical and family-level floating-exception phases are
complete. The remainder/reduction family identified by this checkpoint has now
also been promoted: `fmod`, `remainder`, `remquo`, and their binary32 variants
isolate internal reduction flags and deliberately raise `FE_INVALID` for their
existing domain results.

The strongest remaining range-family fenv gap is now the power-of-two scaling
layer. `scalbn`/`ldexp` and their binary32 variants already expose `ERANGE` for
finite overflow and inexact tiny results, but still need deliberate public
`FE_OVERFLOW | FE_INEXACT` and `FE_UNDERFLOW | FE_INEXACT` propagation plus
sticky-flag and host-fenv evidence. That family should be re-audited against the
latest main before another implementation slice is opened.

`math_errhandling` remains `MATH_ERRNO` until the remaining public range/domain
families have executable exception coverage. Long-double special functions,
complex arithmetic, and globally correctly rounded transcendental guarantees
remain separate phases.
