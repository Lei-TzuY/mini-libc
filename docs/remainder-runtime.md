# Remainder and floating classification runtime

## Phase status

This phase is complete for the current binary32/binary64 mini-libc math target.
The public math surface now includes type-sensitive floating classification and
ordered-comparison macros together with `fmod`, `remainder`, and `remquo`
families for `double` and `float`.

This is an executable capability checkpoint, not a claim of complete C math
conformance. Long-double classification, floating-environment exceptions,
rounding-mode integration, and the remaining special-function families remain
outside this phase.

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

The phase adds:

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

## Special values and errno

Current deterministic policy:

- a NaN operand is propagated without changing `errno`;
- finite `x` with infinite `y` returns `x`;
- signed zero in `x` is preserved;
- infinite `x`, or a zero divisor, is a domain error: `errno = EDOM` and a quiet
  NaN is returned;
- `remquo` clears its quotient output to zero before a domain-result return;
- valid finite operations restore the caller's incoming `errno`, including
  cases where internal decomposition/scaling helpers are used.

No floating exception flags are claimed because the project does not yet expose
`<fenv.h>` or `math_errhandling` exception support beyond `MATH_ERRNO`.

## Executable evidence

`tests/remainder_probe.c` is a freestanding static executable that checks:

- binary32 and binary64 zero/subnormal/normal/infinity/NaN classification;
- type-sensitive float-subnormal handling;
- single evaluation of public macros;
- ordered and unordered comparisons;
- signed-zero behavior;
- basic and huge-quotient `fmod` reduction;
- nearest-even `remainder` ties;
- seven-bit `remquo` values and quotient sign;
- domain errors and NaN payload propagation.

`tests/remainder_differential.c` compiles the production remainder object under
renamed public symbols and compares it with the host libm on a controlled corpus
covering signs, ties, large exponent gaps, minimum normal values, and
subnormals. `fmod`, `remainder`, and `remquo` residuals are required to match
bit-for-bit on that corpus.

Pinned tiny-c compiles every production math C object plus the extended
`tests/tiny_math_integration.c`. The same executable exercises `_Generic`
classification, float subnormals, huge-quotient reduction, nearest-even
remainder, and `remquo`, and is linked/run through both GNU `ld` and the pinned
mini-elf-toolchain.

## Architecture boundary

The phase is intentionally split into two archive objects:

- `math_classify.o`: IEEE-754 binary32/binary64 classification, sign, and
  ordered-comparison helpers;
- `math_remainder.o`: finite reduction and the public remainder families,
  relying on classification plus the existing decomposition/scaling substrate.

This keeps classification cheap to link while allowing the remainder engine to
reuse already-established math primitives.

## Next frontier

The remainder/classification checkpoint closes the major ordinary real-math
surface identified after the hyperbolic phase. The next promotion should move
up a level rather than grow another batch of remainder vectors.

The highest-value remaining real-math frontier is a coherent special-function
layer. A good next vertical slice is the error-function family (`erf`/`erfc`
and float variants) with piecewise small/central/tail approximations, explicit
NaN/infinity/signed-zero semantics, controlled host differential bounds, and
pinned tiny-c/mini-elf execution. That can then provide a clean numerical and
verification base before taking on the more demanding gamma family.

Long-double support, `<fenv.h>`, complex arithmetic, and globally
correctly-rounded transcendental claims remain separate architectural phases.
