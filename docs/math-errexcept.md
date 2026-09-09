# Repository-wide math error/exception closure

## Phase status

mini-libc now advertises both standard C math error-reporting mechanisms:

```c
math_errhandling == (MATH_ERRNO | MATH_ERREXCEPT)
```

This promotion closes the family-by-family floating-exception work that reached
scaling in PR #107. It does not add a new numerical algorithm. The result and
`errno` contracts established by the existing math runtime remain unchanged;
the closure claim is that public math error classes covered by mini-libc's
bounded real-math surface also have a deliberate public `FE_*` result-class
contract where an ISO C math error is present.

`MATH_ERREXCEPT` is an error-reporting advertisement. It is not a claim of full
IEC 60559 / Annex F conformance, globally correctly rounded libm results,
rounding-mode-sensitive behavior for every routine, signaling-NaN behavior
identical to a host libm, long-double support, or trap-control semantics.

## Error-family inventory

The repository-wide claim is backed by the previously promoted executable
families rather than by one synthetic mega-implementation:

- `rint*`, `nearbyint*`, `lrint*`, and `llrint*` integrate active rounding and
  deliberate `FE_INEXACT` / `FE_INVALID` behavior;
- `exp*` and `log*` map domain, pole, overflow, underflow, and inexact result
  classes onto `FE_INVALID`, `FE_DIVBYZERO`, `FE_OVERFLOW`,
  `FE_UNDERFLOW`, and `FE_INEXACT` as applicable;
- `pow*` deliberately maps negative-base domain errors, zero-base poles, and
  finite range failures;
- `sqrt*`, `asin*`, `acos*`, `acosh*`, and `atanh*` deliberately map domain
  and pole classes;
- the hyperbolic family maps finite overflow and inverse-hyperbolic domain /
  pole classes;
- `tgamma*` and `lgamma*` map poles, domains, overflow, and underflow result
  classes;
- `sin*`, `cos*`, and `tan*` deliberately map their bounded large-argument /
  non-finite domain class and expose ordinary inexact evaluation;
- `erf*` / `erfc*` map their bounded range classes;
- `fmod*`, `remainder*`, and `remquo*` preserve exact finite reduction while
  mapping invalid divisor/infinite-dividend classes;
- `scalbn*` / `ldexp*` map finite overflow and inexact tiny results, including
  round-to-zero, to the corresponding overflow/underflow plus inexact flags.

Each family keeps its own focused freestanding probe, hosted fenv interop test,
and numerical differential where applicable. Those tests remain the
family-level source of truth; the closure phase does not replace them with one
coarse test.

## Remaining exact/basic surface

The public operations that do not define mini-libc domain/range/pole errors are
also part of the closure audit. The new closure probes deliberately exercise:

- bit transforms (`fabs*`, `copysign*`);
- `fmin*` / `fmax*` NaN selection and signed-zero behavior;
- classification/predicate paths;
- exact integral `trunc*`, `floor*`, `ceil*`, and `round*` calls;
- `frexp*` decomposition;
- exact `modf*` splitting.

The audit checks that exact/basic calls preserve an existing sticky exception
set and do not invent a new public exception merely because the implementation
uses floating-point registers. It also passes a signaling-NaN bit pattern
through the bit-oriented/basic paths and verifies the documented mini-libc
behavior is exception-clean for those selected operations. This is a bounded
mini-libc implementation contract, not a general signaling-NaN conformance
claim for every C implementation.

The same exact/basic set is executed while `FE_UPWARD` is active. The purpose is
to prove that operations whose result is defined by bit manipulation or an exact
identity do not accidentally change because the caller changed the environment.
It does not imply that every approximate transcendental routine has been made
rounding-direction-sensitive.

## Closure executable

`tests/fenv_closure_probe.c` is a freestanding static executable. It verifies:

- the combined `math_errhandling` advertisement;
- sticky-flag preservation through exact/basic and signaling-NaN bit paths;
- exception-clean exact/basic behavior under `FE_UPWARD`;
- representative repository-wide error classes:
  `sqrt(-1)` -> `EDOM + FE_INVALID`,
  `log(0)` -> `ERANGE + FE_DIVBYZERO`,
  finite `exp` overflow -> `ERANGE + FE_OVERFLOW | FE_INEXACT`, and
  inexact tiny scaling -> `ERANGE + FE_UNDERFLOW | FE_INEXACT`;
- additive sticky flags after exact operations.

`tests/fenv_closure_interop.c` links renamed production basic/decomposition
objects and lets the host `<fenv.h>` observe the real hardware exception state.
It independently verifies signaling-NaN/basic cleanliness and exact behavior
under an active directed rounding mode.

`tests/math_probe.c` now treats the combined macro as part of the normal
freestanding math ABI. Pinned tiny-c compiles and executes the same combined
header contract through `tests/tiny_math_integration.c`; both GNU `ld` and the
pinned mini-elf-toolchain execute that integration binary. The closure probe is
also included in host-libc independence inspection.

## Claim boundary: rounding modes

The global `MATH_ERREXCEPT` promotion does not erase previously documented
numerical boundaries. The scaling family now follows the active rounding mode
for inexact binary64/binary32 subnormal and round-to-zero creation. Exact
normal/subnormal scaling remains mode-independent, and tininess is classified
after rounding so a tiny mathematical result promoted to the minimum normal
value raises only `FE_INEXACT`.

This is still not a blanket Annex F rounding-mode claim. In particular, finite
`scalbn*` / `ldexp*` overflow retains the established signed-infinity endpoint
under every public mode, and bounded polynomial/series implementations remain
governed by their documented numerical error envelopes. `math_errhandling`
describes how math errors are reported, not a promise that every approximate
result is correctly rounded under every active direction.

## Phase promotion

Repository-wide math error reporting and active-rounding tiny scaling are now
completed baselines. More half-way scaling vectors or repeated domain/range
flags would be low-value farming.

The strongest next floating-environment promotion is a coherent numerical
rounding-mode audit across the remaining result kernels. Finite-overflow endpoint
selection in `scalbn*` / `ldexp*` is an explicit first gap, but it should be
audited together with other routines whose bounded implementation still
hard-codes nearest-even or otherwise ignores the active direction rather than
split into a wrapper-only micro-PR. A promotion should make the selected result
bits depend deliberately on all four public rounding modes, preserve existing
`errno` and sticky `FE_*` contracts, and continue through freestanding,
host-fenv, pinned tiny-c, and mini-elf executable evidence.
