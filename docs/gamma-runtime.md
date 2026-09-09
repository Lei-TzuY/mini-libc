# Gamma runtime

## Phase status

This phase is complete for the current binary32/binary64 mini-libc math target.
The public `<math.h>` surface includes `tgamma`, `lgamma`, `tgammaf`, and
`lgammaf` with freestanding, controlled host differential, pinned tiny-c, and
mini-elf executable evidence. The family is now also integrated with the public
x86-64 `<fenv.h>` exception environment.

This is a bounded numerical implementation. It does not claim globally
correctly-rounded gamma values, long-double variants, or a public `signgam`
object.

## Shared numerical substrate

`src/math/gamma.c` owns one positive log-gamma core and one reflection path.
`tgamma` does not maintain an independent approximation: it obtains
`log(|Gamma(x)|)` and the reflection sign from the shared core, then uses the
existing mini-libc `exp` runtime to recover the magnitude.

For positive arguments the implementation uses a nine-term Lanczos
approximation with `g = 7`. The logarithmic form is evaluated directly,

`log(sqrt(2*pi)) + (z + 1/2) * log(t) - t + log(sum)`,

so large positive arguments do not first construct an enormous intermediate
Gamma value merely to take its logarithm.

For `x < 0.5`, the implementation uses the reflection identity

`Gamma(x) * Gamma(1-x) = pi / sin(pi*x)`.

The reflection helper deliberately does not send a potentially huge `pi*x`
argument through mini-libc's bounded trigonometric reducer. Every representable
binary64 noninteger has magnitude below `2^52`; the helper therefore separates
the integer parity and fractional part first, mirrors the fraction into
`(0, 0.5]`, and calls the existing `sin` runtime only on an angle no larger than
`pi/2`. At magnitudes at or above `2^52`, representable binary64 values are
integral and the negative side is consequently a Gamma pole.

Internal `sin`, `log`, and `exp` calls preserve the caller's incoming `errno`
when the enclosing Gamma operation succeeds. Public range/domain status is
owned by the Gamma layer rather than leaking an implementation dependency's
errno policy.

## Public result, errno, and floating-exception behavior

The deterministic mini-libc contract is:

- NaN payloads pass through unchanged without adding Gamma-owned severe flags;
- `tgamma(+inf) = +inf` and preserves incoming `errno` and existing exception
  state;
- `tgamma(-inf)` returns a quiet NaN, sets `EDOM`, and raises `FE_INVALID`;
- `tgamma(+0)` returns `+inf`, sets `ERANGE`, and raises `FE_DIVBYZERO`;
- `tgamma(-0)` returns `-inf`, sets `ERANGE`, and raises `FE_DIVBYZERO`;
- negative integer poles return a quiet NaN from `tgamma`, set `EDOM`, and
  raise `FE_INVALID`;
- finite noninteger negative arguments obtain their sign from reflection;
- finite `tgamma` overflow sets `ERANGE` and raises
  `FE_OVERFLOW | FE_INEXACT`;
- a finite `tgamma` result in the binary64 subnormal/zero range sets `ERANGE`
  and raises `FE_UNDERFLOW | FE_INEXACT`;
- `lgamma(+inf)` and `lgamma(-inf)` return `+inf` while preserving incoming
  `errno` and existing exception state;
- zero and negative-integer poles return `+inf` from `lgamma`, set `ERANGE`,
  and raise `FE_DIVBYZERO`;
- a finite `lgamma` overflow sets `ERANGE` and raises
  `FE_OVERFLOW | FE_INEXACT`;
- successful finite normal calls preserve incoming `errno`; ordinary
  transcendental evaluation may leave `FE_INEXACT` when the computed result is
  not exact.

The Gamma layer raises only the class that belongs to the public result/error
classification and does not clear pre-existing exception flags. Clean special
paths therefore preserve caller-owned sticky flags.

`lgamma` returns `log(|Gamma(x)|)`. This phase intentionally does not expose the
historical non-ISO `signgam` global; callers that require the sign of Gamma for a
negative noninteger should use `tgamma` or maintain their own sign policy.

The binary32 variants reuse the binary64 core and then apply an explicit
narrowing boundary. A finite nonzero binary64 result that narrows to binary32
infinity raises `FE_OVERFLOW | FE_INEXACT`; a result that narrows into the
binary32 subnormal/zero range raises `FE_UNDERFLOW | FE_INEXACT`. Both remain
`ERANGE` results. NaN payload handling is performed before widening so public
float special-value behavior remains deterministic.

## Archive boundary

Gamma lives in the independent `math_gamma.o` archive member. It reuses the
established `sin`, `log`, `exp`, and fenv substrate but does not pull the
error-function implementation itself. Conversely, a program that only needs
`erf`/`erfc` does not pull Gamma code merely because both families are wired
through `mk/math-special.mk`.

## Executable evidence

The freestanding `gamma_probe` remains the numerical/result checkpoint and
covers positive Lanczos values, negative reflection, `lgamma` magnitude
semantics, range results, poles, infinities, NaNs, and binary32 narrowing.

`gamma_differential` compiles the production Gamma object under renamed symbols
and compares it with the host libm over controlled positive, reflected-negative,
large-magnitude, and binary32 corpora. The tolerances document a bounded
numerical contract rather than a correctly-rounded claim. This differential is
kept unchanged by the fenv promotion so exception plumbing cannot silently
alter numerical behavior.

The dedicated `fenv_gamma_probe` is a freestanding static executable that checks
mini-libc result + errno + FE_* behavior for:

- zero and negative-integer poles;
- `tgamma(-inf)` domain behavior;
- binary64 overflow and underflow;
- binary32 narrowing overflow and underflow;
- ordinary inexact evaluation; and
- preservation of pre-existing sticky flags across clean infinity paths.

`fenv_gamma_interop` compiles the production Gamma object under renamed symbols
and observes its x87/MXCSR flags through the host `<fenv.h>` ABI. It deliberately
checks hardware-visible results and exception flags rather than host-libc
`errno`, because the renamed production object retains mini-libc's independent
errno storage. The freestanding and tiny-c probes own the mini-libc errno
contract.

Pinned tiny-c compiles the production Gamma source and executes the dedicated
`tiny_fenv_gamma` program. The same Gamma/fenv executable is linked and run
through GNU `ld` and the pinned mini-elf-toolchain, and is included in the
host-libc-independence inspection.

## Phase boundary and promotion

Gamma's existing result/errno semantics are now promoted into the public
floating environment without changing the Lanczos/reflection numerical
algorithms. Further Gamma vectors or a `signgam` compatibility global would be
low-value micro-expansion at this checkpoint.

The remaining high-value math semantic work is family-level FE_* propagation in
math surfaces that still expose errno/domain/range behavior without equivalent
public exception reporting. The remainder/reduction family, trigonometric
range/domain paths, and error-function range paths should be audited and
promoted as coherent families rather than through one-function micro-PRs.

Long-double math, complex arithmetic, locale-sensitive numerical behavior, and
globally correctly-rounded transcendental guarantees remain separate phases.
