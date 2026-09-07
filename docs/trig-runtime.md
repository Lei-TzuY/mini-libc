# Trigonometric runtime ABI and phase status

mini-libc now has a bounded executable real trigonometric runtime for x86-64
binary32 and binary64. The public forward-trigonometric surface is:

```c
double sin(double x);
float sinf(float x);
double cos(double x);
float cosf(float x);
double tan(double x);
float tanf(float x);
```

The inverse layer is documented separately in `docs/inverse-trig-runtime.md` and
adds `atan`/`atan2`/`asin`/`acos` plus their binary32 variants.

The implementation is freestanding, links no host libm, and uses no compiler
math builtins. All six forward entry points share one binary64 argument-reduction
layer and one pair of reduced sine/cosine polynomial kernels. The float entry
points reuse that numerical core after preserving binary32 NaN payloads
explicitly.

## Bounded range reduction

For finite inputs with `|x| <= 2^20`, mini-libc performs a Cody-Waite style
`pi/2` reduction. It multiplies by `2/pi`, rounds to the nearest integral
quadrant count, and subtracts a split `pi/2` high/low pair. The reduced argument
is kept in the neighborhood of `[-pi/4, pi/4]`. The quadrant count modulo four
then reconstructs the signs and sine/cosine interchange for the original
argument.

This phase deliberately does not claim a Payne-Hanek reducer or globally correct
large-argument trigonometry. A finite input with magnitude greater than `2^20`
returns a quiet NaN and sets `errno = EDOM`. This is a documented bounded
mini-libc extension rather than a claim of complete C/libm conformance. The
boundary exists so numerical error from reducing arbitrarily large binary64
arguments is not hidden behind an unsupported accuracy claim.

## Shared reduced kernels

The reduced sine and cosine kernels are evaluated as odd/even polynomials in
`x^2`. `sin` and `cos` select or negate those two kernel results according to the
quadrant. `tan` deliberately consumes the same reconstructed sine/cosine pair and
returns their quotient instead of growing a third independent range reducer or
polynomial subsystem.

This architecture gives the three forward functions one argument-reduction truth
source and one special-value policy. The inverse-trigonometric layer remains
independent of this large-angle reducer: it owns a separate bounded atan kernel
and quadrant engine rather than coupling inverse functions to the forward
polynomials.

The current implementation is numerically bounded, not correctly rounded.
Controlled host-libm differential tests cover representative positive and
negative values from small arguments through the declared `2^20` range. The
published tolerances are intentionally looser than the observed error and are
part of the test contract rather than a global ULP guarantee. `tan` has a wider
relative tolerance because division amplifies sine/cosine error near poles.

## Special values and errno

Supported finite calls preserve an existing errno value.

- `sin(+0)` and `tan(+0)` return `+0`.
- `sin(-0)` and `tan(-0)` preserve `-0`.
- `cos(+0)` and `cos(-0)` return exactly `1`.
- binary64 NaNs are returned unchanged, preserving payload/sign bits.
- binary32 NaNs passed to `sinf`/`cosf`/`tanf` are returned before widening, so
  their binary32 payload bits are preserved as well.
- positive or negative infinity returns a quiet NaN and sets `EDOM`.
- finite inputs outside the declared reduction bound return a quiet NaN and set
  `EDOM`.

`math_errhandling` remains `MATH_ERRNO`; no floating-point exception or rounding
mode interface is claimed by this phase.

## Executable evidence

The freestanding `trig_probe` is linked only against mini-libc. It covers:

- signed-zero behavior;
- familiar `pi/6`, `pi/4`, `pi/3`, `pi/2`, and `pi` quadrant cases;
- medium and large supported values including `1234.5` and `1,000,000`;
- binary32 entry points;
- errno preservation on successful calls;
- binary64 and binary32 NaN payload passthrough;
- infinity domain failure;
- the explicit finite large-argument boundary.

The hosted `trig_differential` compiles production `trig.c` under renamed symbols
and compares all six functions with the host libm on a controlled corpus spanning
roughly `[-10^6, 10^6]`. Sine/cosine and tangent use separately declared
relative/absolute tolerances. The differential additionally checks mini-libc's
own errno storage and special-value behavior rather than inferring those policies
from the host.

Pinned tiny-c compiles the production trig object together with the rest of
mini-libc and executes ordinary and binary32 trigonometric calls in
`tiny_math_integration`. The same object archive is then linked and run through
both GNU `ld` and the pinned mini-elf-toolchain. The three-repo gate therefore
proves executable behavior through the pinned compiler and linker rather than
only host GCC/Clang builds.

## Phase boundary and promotion

The bounded forward-trigonometric phase is complete, and the inverse layer is
now implemented as a separate shared angular subsystem. Together they provide
executable binary32/binary64 forward and inverse real trigonometric coverage,
without implying a full large-argument Payne-Hanek reducer, long-double
variants, complex math, floating-environment control, or global correctly-rounded
guarantees.

Future math work should now promote beyond wrapper-level trigonometric breadth.
The strongest candidates are a shared hyperbolic layer reusing the existing
`exp`/`log`/`sqrt` substrate, or a coherent remainder/classification subsystem.
A fresh live repository audit should select between those architectural gaps
before another implementation slice is opened.
