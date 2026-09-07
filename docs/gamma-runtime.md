# Gamma runtime

## Phase status

This phase is complete for the current binary32/binary64 mini-libc math target.
The public `<math.h>` surface now includes `tgamma`, `lgamma`, `tgammaf`, and
`lgammaf` with freestanding, controlled host differential, pinned tiny-c, and
mini-elf executable evidence.

This is a bounded numerical implementation. It does not claim globally
correctly-rounded gamma values, long-double variants, a public `signgam` object,
or floating-environment exception reporting.

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
status.

## Public behavior

The deterministic mini-libc contract is:

- NaN payloads pass through unchanged;
- `tgamma(+inf) = +inf` and preserves incoming `errno`;
- `tgamma(-inf)` returns a quiet NaN and sets `EDOM`;
- `tgamma(+0)` returns `+inf` with `ERANGE`;
- `tgamma(-0)` returns `-inf` with `ERANGE`;
- negative integer poles return a quiet NaN from `tgamma` with `EDOM`;
- finite noninteger negative arguments obtain their sign from reflection;
- finite `tgamma` overflow or a subnormal/zero result sets `ERANGE`;
- `lgamma(+inf)` and `lgamma(-inf)` return `+inf` while preserving incoming
  `errno`;
- zero and negative-integer poles return `+inf` from `lgamma` with `ERANGE`;
- successful finite normal calls preserve incoming `errno`.

`lgamma` returns `log(|Gamma(x)|)`. This phase intentionally does not expose the
historical non-ISO `signgam` global; callers that require the sign of Gamma for a
negative noninteger should use `tgamma` or maintain their own sign policy.

The binary32 variants reuse the binary64 core and then apply an explicit
narrowing boundary. A finite nonzero binary64 result that narrows to binary32
infinity, a subnormal, or zero is a binary32 range result and sets `ERANGE`.
NaN payload handling is performed before widening so public float special-value
behavior remains deterministic.

No floating exception flags are claimed. `math_errhandling` remains
`MATH_ERRNO`, and `<fenv.h>` is not yet part of the runtime.

## Archive boundary

Gamma lives in the independent `math_gamma.o` archive member. It reuses the
established `sin`, `log`, and `exp` substrate but does not pull the error-function
implementation itself. Conversely, a program that only needs `erf`/`erfc` does
not pull Gamma code merely because both families are wired through
`mk/math-special.mk`.

## Executable evidence

The freestanding `gamma_probe` is linked only against mini-libc and covers:

- positive Lanczos values;
- negative noninteger reflection and alternating sign;
- `lgamma` magnitude semantics;
- positive overflow and negative underflow;
- positive and negative zero poles;
- negative-integer poles;
- infinities and NaN payload propagation;
- binary32 values and narrowing overflow.

`gamma_differential` compiles the production Gamma object under renamed symbols
and compares it with the host libm over controlled positive, reflected-negative,
large-magnitude, and binary32 corpora. Its production dependencies are the
renamed mini-libc trig, exp/log, and decomposition objects; host libm is used as
the reference, not as an implementation dependency. The tolerances document a
bounded numerical contract rather than a correctly-rounded claim.

Pinned tiny-c compiles the production Gamma source and executes positive,
reflected-negative, float, pole, and overflow cases from
`tests/tiny_math_integration.c`. The same executable is linked and run through
GNU `ld` and the pinned mini-elf-toolchain.

## Phase boundary and promotion

The dedicated real special-function layer now contains error-function and Gamma
families on top of the existing real-math substrate. Farming additional Gamma
vectors or a `signgam` compatibility global would not be the strongest next
architectural step.

The next high-value promotion is a floating-environment foundation: a bounded
x86-64 `<fenv.h>` runtime should expose rounding-mode state and floating
exception-status operations coherently across the SSE/MXCSR and x87 environment,
with executable GCC, Clang, pinned tiny-c, and mini-elf evidence. That substrate
would make later `rint`/`nearbyint`, exception-aware `math_errhandling`, and
rounding-sensitive numerical work honest rather than simulated in isolated math
functions.

Long-double math, complex arithmetic, locale-sensitive numerical behavior, and
globally correctly-rounded transcendental guarantees remain separate phases.
