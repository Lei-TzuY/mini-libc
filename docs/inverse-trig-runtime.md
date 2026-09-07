# Inverse trigonometric runtime ABI and phase status

mini-libc now has a bounded executable inverse-trigonometric layer for x86-64
binary32 and binary64. The public surface is:

```c
double atan(double x);
float atanf(float x);
double atan2(double y, double x);
float atan2f(float y, float x);
double asin(double x);
float asinf(float x);
double acos(double x);
float acosf(float x);
```

The implementation is freestanding, links no host libm, and uses no compiler
math builtins. The four binary64 entry points share one angular kernel and one
quadrant engine rather than growing independent inverse-polynomial subsystems.
The binary32 variants reuse that core after preserving float NaN payloads before
widening.

## Shared atan kernel

`atan` first classifies sign, zero, infinity, and NaN from the IEEE-754 binary64
representation. Finite positive magnitudes are reduced with the standard
`tan(pi/8)` / `tan(3pi/8)` identities:

- small values are evaluated directly;
- middle values use `pi/4 + atan((x - 1) / (x + 1))`;
- large values use `pi/2 - atan(1 / x)`.

That keeps the reduced kernel argument within approximately
`[-(sqrt(2)-1), sqrt(2)-1]`. The reduced kernel is one bounded odd arctangent
series, so all finite magnitudes consume the same numerical approximation layer.
The result is then reconstructed with the original sign.

This is a bounded numerical implementation, not a correctly-rounded libm claim.
Controlled host-libm differential tests define the actual tolerance contract.

## atan2 quadrant and special-value engine

`atan2` owns the two-argument axis/quadrant semantics. For finite nonzero inputs
it forms only a ratio of the smaller magnitude to the larger magnitude, avoiding
spurious overflow from an unrestricted `y / x`. The shared positive atan kernel
produces the acute angle and `atan2` reconstructs the requested quadrant.

The special matrix is explicit:

- signed-zero `y` is preserved on the positive x-axis;
- signed-zero `y` with negative `x` produces signed `pi`;
- nonzero `y` with zero `x` produces signed `pi/2`;
- combinations of infinite `x` and `y` produce the appropriate `pi/4`,
  `3*pi/4`, `pi/2`, `pi`, or signed-zero boundary result;
- if `y` is a NaN it is returned first; otherwise an `x` NaN is returned.

These successful classifications do not modify errno.

## asin and acos composition

`asin` and `acos` do not introduce separate inverse-trigonometric polynomial
kernels. They combine the existing hardware-backed square-root runtime with
`atan2`:

- `asin(x)` uses `atan2(|x|, sqrt((1-|x|)*(1+|x|)))` and restores the input
  sign;
- `acos(x)` uses `atan2(sqrt((1-x)*(1+x)), x)`.

The products avoid the direct `1 - x*x` cancellation pattern while preserving a
simple bounded composition. Exact endpoints are handled separately:
`asin(+/-1) = +/-pi/2`, `acos(1) = 0`, and `acos(-1) = pi`.

A finite argument with `|x| > 1` returns a quiet NaN and sets `errno = EDOM`.
NaNs pass through without changing errno. Signed zero is preserved by `asin`.

## Binary32 behavior

`atanf`, `atan2f`, `asinf`, and `acosf` widen finite binary32 inputs through the
same binary64 numerical core and narrow the final result. A binary32 NaN is
returned before widening so its binary32 payload/sign bits are not rewritten by
a binary64 round trip.

No long-double, complex-math, floating-exception, or alternate-rounding-mode
contract is added by this phase.

## Executable evidence

The freestanding `inverse_trig_probe` is linked only against mini-libc and checks:

- signed-zero behavior for `atan`, `atan2`, and `asin`;
- representative small, medium, and large finite `atan` inputs;
- all four ordinary `atan2` quadrants;
- zero-axis and infinity combinations;
- `asin`/`acos` interior values and exact `+/-1` boundaries;
- `EDOM` for finite `asin`/`acos` inputs outside `[-1,1]`;
- binary64 and binary32 NaN payload passthrough;
- binary32 entry points;
- preservation of an existing errno value on successful operations.

The hosted `inverse_trig_differential` compiles production
`inverse_trig.c` under renamed symbols and compares all eight public functions
with host libm across wide `atan` magnitudes, extreme `atan2` ratios and
quadrants, dense endpoint-oriented `asin`/`acos` values, and representative
binary32 inputs. Explicit relative/absolute tolerances are part of the test
contract; no global ULP guarantee is claimed.

Pinned tiny-c compiles `inverse_trig.c` with the rest of mini-libc and executes
ordinary and binary32 inverse-trigonometric calls through
`tiny_math_integration`. Both GNU `ld` and the pinned mini-elf-toolchain link and
run the same executable, so the new layer is covered by the existing three-repo
gate rather than only host GCC/Clang builds.

## Phase boundary and promotion

The forward and inverse real-trigonometric layers are now both executable at
their documented bounded contracts. The remaining math surface is still far
from a complete libm: there is no long-double family, complex math, public
floating environment, remainder family, hyperbolic family, error/gamma family,
or globally correctly-rounded guarantee.

The next repository-wide math promotion should be chosen from those remaining
architectural gaps rather than by adding aliases or wrapper-only functions. A
strong next candidate is a shared hyperbolic layer that reuses the existing
`exp`/`log`/`sqrt` substrate for `sinh`/`cosh`/`tanh` and inverse hyperbolic
functions while defining overflow/domain/range behavior and binary32 narrowing.
A fresh live audit should still compare that value against a coherent remainder
or classification subsystem before implementation begins.
