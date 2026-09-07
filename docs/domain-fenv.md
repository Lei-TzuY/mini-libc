# Domain/root floating-exception runtime

## Phase status

The domain/root math family is now deliberately connected to mini-libc's public
floating environment. This phase promotes existing result and `errno` classes;
it does not change the numerical algorithms for square root, inverse
trigonometric functions, or inverse hyperbolic functions.

The promoted family is:

- `sqrt` / `sqrtf`;
- `asin` / `asinf`;
- `acos` / `acosf`;
- `acosh` / `acoshf`;
- `atanh` / `atanhf`.

This remains family-level exception coverage. `math_errhandling` stays
`MATH_ERRNO`; mini-libc still does not advertise repository-wide
`MATH_ERREXCEPT` support.

## Exception contract

Existing exact identity paths remain exception-clean and preserve an existing
`errno` value:

- `sqrt(4)` returns `2`;
- `sqrt(-0)` preserves negative zero;
- `asin(0)` returns signed zero;
- `acos(1)` returns `0`;
- `acosh(1)` returns `0`;
- `atanh(0)` returns signed zero.

Those exact calls do not clear pre-existing sticky flags. A flag raised before
an exact call remains visible afterwards.

Domain and pole classes now map deliberately onto the public environment:

- finite negative nonzero `sqrt` / `sqrtf` inputs preserve the existing
  `EDOM + quiet NaN` contract and raise `FE_INVALID`;
- `asin` / `asinf` and `acos` / `acosf` with `|x| > 1` preserve the existing
  `EDOM + quiet NaN` contract and raise `FE_INVALID`;
- `acosh` / `acoshf` with `x < 1` preserve the existing `EDOM + quiet NaN`
  contract and raise `FE_INVALID`;
- `atanh` / `atanhf` with `|x| > 1` preserve `EDOM + quiet NaN` and raise
  `FE_INVALID`;
- `atanh` / `atanhf` at `+1` or `-1` preserve the existing
  `ERANGE + signed infinity` pole result and raise `FE_DIVBYZERO`.

The implementation uses `feraiseexcept`; it does not introduce a private math
exception store.

Ordinary irrational/interior results are not assigned a software approximation
heuristic. Their normal arithmetic path leaves the hardware `FE_INEXACT` state
visible. Representative executable cases include `sqrt(2)`, `asin(0.5)`,
`acos(0.5)`, `acosh(2)`, and `atanh(0.5)`.

## Numerical boundary

No approximation polynomial, threshold, range reduction, or result
classification changed in this phase.

In particular:

- positive `sqrt` still delegates to the existing x86-64 hardware square-root
  primitive;
- `asin` / `acos` retain their existing `sqrt` + `atan2` composition;
- `acosh` / `atanh` retain the existing `sqrt` / `log` composition and small
  kernels;
- NaN handling remains unchanged;
- float wrappers still reuse the binary64 implementations where they did
  before this phase.

The new production changes are restricted to `<fenv.h>` dependencies and
explicit domain/pole exception side effects. Existing numerical probes and
host differentials remain the numerical contract.

## Executable evidence

The freestanding `fenv_domain_probe` verifies:

- exact-clean square-root and inverse-domain identities;
- signed negative zero through `sqrt`;
- ordinary `FE_INEXACT` visibility;
- `EDOM + FE_INVALID` across root, inverse-trigonometric, and inverse-hyperbolic
  domains;
- `ERANGE + FE_DIVBYZERO` for positive and negative `atanh` poles;
- binary32 wrapper propagation;
- preservation of a pre-existing sticky exception across exact calls.

The hosted `fenv_domain_interop` compiles renamed production implementations and
lets the system `<fenv.h>` observe the real x87/MXCSR state. It exercises the
same exact, inexact, domain, pole, and float-wrapper classes, proving that these
side effects are hardware-visible rather than test-private bookkeeping.

Pinned tiny-c compiles and runs the promoted family inside
`tiny_fenv_integration.c`, including exact square root, domain failures, the
`atanh` pole, and an ordinary inexact result. The same source set is linked and
executed through GNU `ld` and the pinned mini-elf-toolchain.

## Phase boundary and promotion

The domain/root exception phase is complete. Adding more inputs to the same
result classes would now be low-value repetition.

The next fenv promotion should continue by semantic coverage rather than API
count. Strong candidates are existing direct range and pole branches that can
currently return classified results without passing through an already
exception-aware substrate. In particular, `sinh` / `cosh` finite overflow and
remaining special-function domain/range families should be audited before
claiming broader exception coverage.

A follow-on slice should preserve each family's established numerical and
`errno` contract, add only the missing public `FE_*` effects, and keep the same
freestanding + host-fenv + pinned tiny-c + mini-elf evidence standard.

`math_errhandling` must remain `MATH_ERRNO` until exception propagation is broad
enough that `MATH_ERREXCEPT` describes the library as a whole rather than a
promoted subset.
