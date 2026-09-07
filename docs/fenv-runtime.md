# Floating environment runtime

## Phase status

The bounded x86-64 floating-environment substrate is complete and now has a
real rounding-sensitive math consumer. mini-libc exposes `<fenv.h>` with
synchronized x87 and SSE/MXCSR rounding control, sticky exception status, and
environment save/restore operations. The public rounding family
`rint`/`rintf`, `nearbyint`/`nearbyintf`, `lrint`/`lrintf`, and
`llrint`/`llrintf` consumes that environment directly rather than relying on a
compiler `FENV_ACCESS` assumption.

The behavior is covered by freestanding GCC/Clang execution, host-fenv
interoperability, pinned tiny-c execution, and the pinned mini-elf-toolchain.
This phase deliberately does not claim that every older math routine raises the
full ISO C floating-exception set. `math_errhandling` therefore remains
`MATH_ERRNO` until exception-aware math coverage is promoted deliberately across
the broader runtime.

## Public surface

The public exception macros are:

- `FE_INVALID`;
- `FE_DIVBYZERO`;
- `FE_OVERFLOW`;
- `FE_UNDERFLOW`;
- `FE_INEXACT`;
- `FE_ALL_EXCEPT`.

The public rounding modes are `FE_TONEAREST`, `FE_DOWNWARD`, `FE_UPWARD`, and
`FE_TOWARDZERO`.

`fexcept_t`, `fenv_t`, and `FE_DFL_ENV` provide the state boundary used by:

- `feclearexcept`, `fegetexceptflag`, `feraiseexcept`, `fesetexceptflag`, and
  `fetestexcept`;
- `fegetround` and `fesetround`;
- `fegetenv`, `feholdexcept`, `fesetenv`, and `feupdateenv`.

The rounding-sensitive `<math.h>` surface is:

- `rint`, `rintf`;
- `nearbyint`, `nearbyintf`;
- `lrint`, `lrintf`;
- `llrint`, `llrintf`.

Successful fenv operations do not modify `errno`. Null state pointers and
unsupported rounding values are rejected with a nonzero return rather than by
inventing an errno contract.

## x86-64 hardware model

The implementation keeps the two floating execution domains coherent:

- x87 control-word bits 10-11 hold the rounding mode;
- MXCSR bits 13-14 hold the SSE rounding mode;
- the low hardware exception-status bits represent the sticky x87 and SSE
  exception state.

The public exception set intentionally excludes the x86 denormal-operand status
bit because ISO C has no corresponding `FE_*` macro. Reads report the union of
public x87 and MXCSR sticky flags. Selected public flag writes update both domains
so later x87 or SSE observation sees the same environment.

`fegetround` compares the x87 and MXCSR rounding fields. If external code has
made them disagree, it returns a nonstandard failure value instead of pretending
that one engine is authoritative. `fesetround` changes both engines together.

The private assembly boundary reads control/status through `fnstcw`, `fnstsw`,
and `stmxcsr`. Environment writes use an aligned `FXSAVE` image, patch only the
x87 control/status and MXCSR fields represented by `fenv_t`, then restore through
`FXRSTOR`. This preserves x87/XMM register contents, x87 stack position and
condition state, and the rest of the saved floating context while applying the
requested public environment.

`fenv_t.__mxcsr` stores the complete MXCSR value obtained from the hardware.
`fesetenv` therefore expects an environment previously produced by `fegetenv` or
the valid `FE_DFL_ENV` object; arbitrary caller-fabricated reserved MXCSR bit
patterns are outside this bounded contract.

## Exception and environment behavior

`feclearexcept`, `feraiseexcept`, and `fesetexceptflag` modify public sticky
status coherently in x87 and MXCSR. `feraiseexcept` is deliberately a status-
setting operation: this phase does not expose exception trap-enable controls and
does not promise synchronous `SIGFPE` delivery for callers that externally
unmask hardware traps.

`feholdexcept` saves the current environment, masks all x87/SSE floating
exceptions, clears sticky exception status, and preserves the current rounding
mode. `feupdateenv` records the currently raised public flags, restores the
supplied environment, then re-raises the recorded flags. `FE_DFL_ENV` restores
x87 control `0x037f`, clear public sticky status, and MXCSR `0x1f80`.

No public control is currently provided for x87 precision mode, DAZ/FTZ policy,
or exception trap masks independently of `feholdexcept`/environment restoration.
The runtime also does not claim compiler support for `#pragma STDC FENV_ACCESS`.

## Rounding-sensitive math behavior

The new rounding family reads the active environment explicitly with
`fegetround`. Binary32 and binary64 values are rounded by inspecting and masking
the IEEE-754 significand bits, so correctness does not depend on whether the C
compiler treats ordinary arithmetic as fenv-sensitive.

All four public rounding modes are implemented. `FE_TONEAREST` uses ties to even;
`FE_UPWARD`, `FE_DOWNWARD`, and `FE_TOWARDZERO` follow their directed rules while
preserving signed zero when the mathematical result is zero. Integral values,
infinities, NaNs, and zeros pass through `rint*`/`nearbyint*` without an
artificial integer conversion.

`rint` and `rintf` explicitly add `FE_INEXACT` when a nonintegral finite input is
rounded. `nearbyint` and `nearbyintf` compute the same active-mode result without
adding `FE_INEXACT`; existing exception flags are preserved.

`lrint`, `lrintf`, `llrint`, and `llrintf` use the same active-mode rounding and
raise `FE_INEXACT` on successful inexact conversions. On this x86-64 LP64 target,
mini-libc defines a deterministic failure policy for the otherwise unspecified
integer result: a finite value outside signed 64-bit range returns the relevant
minimum signed sentinel, sets `errno = ERANGE`, and raises `FE_INVALID`; NaN or
infinity returns the same sentinel, sets `errno = EDOM`, and raises
`FE_INVALID`. Successful representable conversions preserve an existing errno
value.

If external code leaves x87 and MXCSR with incoherent rounding fields,
`fegetround` reports the existing nonstandard failure value. The rounding math
then treats that as an invalid environment, raises `FE_INVALID`, and uses the
same deterministic errno policy rather than silently choosing one hardware
engine.

## Executable evidence

The freestanding `fenv_probe` verifies:

- default-environment restoration;
- real half-ULP arithmetic under upward and downward rounding;
- coherent x87/MXCSR rounding control bits;
- hardware divide-by-zero sticky status;
- explicit raise, clear, save, and restore of public exception flags;
- `fegetexceptflag`/`fesetexceptflag`;
- hold/update environment semantics;
- preservation of an existing `errno` value;
- ties-to-even `rint`/`rintf` behavior;
- upward, downward, and toward-zero rounding results;
- the `rint*` versus `nearbyint*` `FE_INEXACT` distinction;
- `lrint*`/`llrint*` result and inexact behavior;
- finite range failure and NaN invalid-conversion policy.

The hosted `fenv_interop` test builds the production C implementation under
renamed symbols and uses the same assembly state boundary. It verifies both
directions of interoperability: mini-libc rounding/exception changes are visible
through the host `<fenv.h>` implementation, and host changes are visible through
mini-libc.

The separate hosted `fenv_rounding_interop` test compiles the production rounding
object and fenv runtime under renamed symbols. Host `<fenv.h>` directly observes
`FE_INEXACT` raised by mini `rint*`/`lrint*`, verifies that mini `nearbyint*`
leaves a pre-existing overflow flag intact without adding `FE_INEXACT`, and sees
`FE_INVALID` on the deterministic integer-conversion failure paths.

Pinned tiny-c compiles the production fenv-aware rounding C object plus
`tiny_fenv_integration.c`. The executable runs active-mode `rint`, `nearbyint`,
`lrint`, and `rintf` paths in addition to the existing hardware fenv checks. The
same binary is linked and run through GNU `ld` and the pinned mini-elf-toolchain.
The freestanding probe is also included in host-libc independence inspection.

## Archive boundary

The fenv C policy and x86 state primitive remain independent `fenv.o` and
`fenv_asm.o` archive members. Environment-sensitive math lives in a separate
`math_fenv_rounding.o` member. Programs that use unrelated basic math do not pull
the fenv dependency merely because the static archive contains these rounding
functions; the dependency becomes live only when one of the fenv-sensitive math
symbols is referenced.

## Phase boundary and promotion

The environment-to-rounding integration is now an executable baseline. Farming
more `rint` input values or flag combinations would be low-value repetition.
The next coherent promotion is broader exception-aware math propagation: existing
math routines with domain/range behavior should deliberately raise the matching
`FE_INVALID`, `FE_DIVBYZERO`, `FE_OVERFLOW`, `FE_UNDERFLOW`, or `FE_INEXACT`
status where their bounded contract can prove it, while preserving the established
errno behavior and continuing to distinguish exact from inexact paths.

That promotion should proceed as coherent function families with freestanding
and hosted fenv evidence rather than by flipping `math_errhandling` globally.
Only after broad executable coverage demonstrates both errno and exception
semantics should `math_errhandling` advertise `MATH_ERREXCEPT`. Long-double
environment semantics, complex arithmetic, and trap-control extensions remain
separate phases.
