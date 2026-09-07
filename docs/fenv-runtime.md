# Floating environment runtime

## Phase status

This phase is complete for the bounded x86-64 floating-environment target.
mini-libc now exposes `<fenv.h>` with synchronized x87 and SSE/MXCSR rounding
control, sticky exception status, and environment save/restore operations. The
behavior is covered by freestanding GCC/Clang execution, host-fenv interoperability,
pinned tiny-c execution, and the pinned mini-elf-toolchain.

This phase deliberately establishes the environment substrate rather than
claiming that every existing math routine already raises ISO C floating
exceptions. `math_errhandling` remains `MATH_ERRNO` until exception-aware math
coverage is promoted deliberately.

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

Successful operations do not modify `errno`. Null state pointers and unsupported
rounding values are rejected with a nonzero return rather than by inventing an
errno contract.

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

## Executable evidence

The freestanding `fenv_probe` verifies:

- default-environment restoration;
- real half-ULP arithmetic under upward and downward rounding;
- coherent x87/MXCSR rounding control bits;
- hardware divide-by-zero sticky status;
- explicit raise, clear, save, and restore of public exception flags;
- `fegetexceptflag`/`fesetexceptflag`;
- hold/update environment semantics;
- preservation of an existing `errno` value.

The hosted `fenv_interop` test builds the production C implementation under
renamed symbols and uses the same assembly state boundary. It verifies both
directions of interoperability: mini-libc rounding/exception changes are visible
through the host `<fenv.h>` implementation, and host changes are visible through
mini-libc.

Pinned tiny-c compiles the production C runtime plus `tiny_fenv_integration.c`.
The executable performs real half-ULP rounding and exception-status operations;
the same binary is linked and run through GNU `ld` and the pinned
mini-elf-toolchain. The freestanding probe is also included in host-libc
independence inspection.

## Archive boundary

The C policy layer and x86 state primitive live in independent `fenv.o` and
`fenv_asm.o` archive members. Programs that do not reference `<fenv.h>` operations
do not pull the environment runtime solely because they use the math library.

## Phase boundary and promotion

The floating environment is now an executable substrate; farming additional
flag combinations would not be the strongest next step. The next coherent
promotion is to connect rounding-sensitive public math to this environment:
`rint`/`rintf`, `nearbyint`/`nearbyintf`, `lrint`/`lrintf`, and
`llrint`/`llrintf` should honor the active rounding mode, distinguish inexact
behavior where required, define integer-conversion range/invalid semantics, and
run through GCC, Clang, pinned tiny-c, and mini-elf.

Only after broad math functions have explicit exception-status evidence should
`math_errhandling` claim `MATH_ERREXCEPT`. Long-double environment semantics,
complex arithmetic, and trap-control extensions remain separate phases.
