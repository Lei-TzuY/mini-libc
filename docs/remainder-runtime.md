# Remainder and floating classification runtime

## Phase status

This phase is complete for the current binary32/binary64 mini-libc math target.
The public math surface includes type-sensitive floating classification and
ordered-comparison macros together with `fmod`, `remainder`, and `remquo`
families for `double` and `float`.

The remainder/reduction family is also promoted into the public floating
environment. Domain results deliberately raise `FE_INVALID`, valid finite
reduction isolates implementation-detail floating exceptions, and exact
passthrough paths preserve the caller's existing sticky flags. This is a
family-level executable checkpoint, not a claim of complete C math conformance,
long-double support, or repository-wide `MATH_ERREXCEPT` coverage.

## Public classification surface

`<math.h>` exposes:

- `FP_NAN`, `FP_INFINITE`, `FP_ZERO`, `FP_SUBNORMAL`, and `FP_NORMAL`;
- `fpclassify`, `isfinite`, `isinf`, `isnan`, `isnormal`, and `signbit`;
- `isunordered`, `isgreater`, `isgreaterequal`, `isless`, `islessequal`, and
  `islessgreater`.

The current macros are deliberately type-sensitive for the supported
`float`/`double` target. C11 `_Generic` selects a binary32 or binary64 helper
without first promoting a `float` value to `double`, so a binary32 subnormal
remains `FP_SUBNORMAL` rather than being misclassified after widening.

The controlling `_Generic` expressions are unevaluated and the selected helper
receives each public macro operand once. The freestanding regression suite
therefore checks side-effecting expressions as well as ordinary constants.

Classification lives in `src/math/classify.c`, an independent archive object
with no dependency on the transform, decomposition, transcendental, or square
root objects. Programs that only use classification do not pull the remainder
reduction chain into the static link.

## Remainder surface

The phase provides:

- `fmod` / `fmodf`;
- `remainder` / `remainderf`;
- `remquo` / `remquof`.

Finite reduction does not form the quotient through `x / y` and then truncate
it. Instead, positive magnitudes are normalized with the existing `frexp`
substrate and reduced by binary long division. Each quotient bit is generated
by compare/subtract and the residual is reconstructed with `scalbn`.

This keeps very large quotients executable without requiring that the quotient
itself be representable as an exact integer-valued `double`. The regression
surface includes reductions such as `2^900 mod 3`.

`fmod` selects the truncation-toward-zero quotient. `remainder` and `remquo`
select the nearest integer quotient, with exact half cases choosing the even
quotient. For the `|x| < |y|` half decision the implementation compares `|x|`
with `|y| - |x|` instead of constructing `|y| / 2`; this avoids introducing a
rounded half when the divisor is an odd subnormal.

## `remquo` contract

The C contract guarantees at least three low quotient bits with the sign of the
quotient. This implementation deliberately retains seven low quotient bits in
the reduction cursor and returns those seven bits with quotient sign.

The hosted differential therefore compares:

- the remainder value bit-for-bit with the host `remquo` result; and
- only the low three quotient bits plus sign against the host quotient, because
  a host libc is not required to expose the same seven-bit extension.

The freestanding and pinned tiny-c tests additionally pin mini-libc's own
seven-bit behavior, including a huge-quotient case.

## Public errno and floating-exception behavior

The deterministic family contract is:

- a quiet NaN operand is propagated without changing `errno` or adding a new
  exception flag;
- finite `x` with infinite `y` returns `x` without adding a new flag;
- signed zero in `x` is preserved without adding a new flag;
- infinite `x`, or a zero divisor, remains a domain error: `errno = EDOM`, a
  quiet NaN is returned, and `FE_INVALID` is deliberately raised;
- `remquo` clears its quotient output to zero before a domain-result return;
- valid finite operations restore the caller's incoming `errno` and floating
  environment after the internal binary reduction, so helper arithmetic and
  internal `frexp`/`scalbn` work do not leak implementation-detail `FE_*`
  flags;
- the float wrappers apply the same boundary around their widening, finite
  reduction, and final narrowing path;
- deliberate domain flags are additive: a caller's pre-existing sticky
  exception set is preserved and `FE_INVALID` is added.

This is family-level exception coverage. `math_errhandling` remains
`MATH_ERRNO`; mini-libc does not yet advertise repository-wide
`MATH_ERREXCEPT` support until a closure audit verifies the complete public math
surface.

## Executable evidence

`tests/remainder_probe.c` remains the freestanding numerical/result checkpoint.
It checks classification, signed-zero behavior, basic and huge-quotient
reduction, nearest-even ties, seven-bit `remquo`, domain errno, and NaN payload
propagation.

`tests/remainder_differential.c` still compiles the production remainder object
under renamed public symbols and compares it with host libm on a controlled
corpus covering signs, ties, large exponent gaps, minimum normal values, and
subnormals. Residuals remain bit-for-bit requirements. After the fenv promotion,
the renamed production object is linked to mini-libc's renamed fenv runtime so
its private `fenv_t` ABI is never mixed with host-libc fenv functions.

`tests/fenv_remainder_probe.c` is a freestanding static executable covering:

- exact valid double and float reductions with no newly visible flag;
- isolation of implementation-detail flags during nontrivial finite reduction;
- zero-divisor and infinite-dividend `FE_INVALID` propagation;
- `remquo` quotient reset on domain results;
- NaN, infinite-divisor, and signed-zero clean paths; and
- additive preservation of caller-owned sticky flags.

`tests/fenv_remainder_interop.c` compiles the production remainder object under
renamed symbols and observes its hardware-visible x87/MXCSR flags through the
host `<fenv.h>` ABI while using mini-libc's fenv implementation internally.

Pinned tiny-c compiles the promoted production remainder runtime and executes
the same family contract from `tests/tiny_fenv_integration.c`. The resulting
executables are linked and run through GNU `ld` and the pinned
mini-elf-toolchain, while the broader tiny math integration continues to pin the
pre-existing numerical results.

## Architecture boundary

The runtime remains split into independent archive objects:

- `math_classify.o`: IEEE-754 binary32/binary64 classification, sign, and
  ordered-comparison helpers;
- `math_decompose.o`: exact `frexp`/`modf` decomposition without a floating
  environment dependency;
- `math_scale.o`: public `scalbn`/`ldexp` power-of-two scaling and its fenv
  range boundary;
- `math_remainder.o`: finite reduction and public remainder families, reusing
  classification, decomposition/scaling, and the public fenv substrate.

The numerical long-division algorithm is unchanged by the fenv promotion. The
boundary remains deliberately at public family entry points: internal floating
work is isolated, then only the exception class owned by the public result is
exposed.

## Next frontier

The remainder numerical/fenv phase and the subsequent scaling range-family fenv
promotion are complete. More ordinary remainder vectors, wrapper-specific
`FE_INVALID` variants, or another one-function exception PR would now be
low-value micro-expansion.

The next architectural phase is a repository-wide `MATH_ERREXCEPT` closure
audit. It must inventory every remaining public exact/basic math family,
identify any signaling-NaN, active-rounding-mode, or implementation-detail
hardware exception that is not yet deliberately owned or isolated, and add
executable regression evidence for genuine gaps. Only after that audit is clean
should `math_errhandling` be considered for promotion from `MATH_ERRNO` to
`MATH_ERRNO | MATH_ERREXCEPT`.

Long-double support, complex arithmetic, and globally correctly-rounded
transcendental guarantees remain separate phases.
