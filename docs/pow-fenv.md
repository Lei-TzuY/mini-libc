# Exception-aware pow runtime

## Phase status

`pow`/`powf` are the third deliberately exception-aware math family in mini-libc,
after the fenv-aware rounding functions and the `exp`/`log` family. The existing
numeric and errno contracts remain authoritative; this phase adds explicit,
sticky `FE_*` effects to the already-defined result classes.

This is still family-level coverage. `math_errhandling` remains `MATH_ERRNO` and
mini-libc does not advertise repository-wide `MATH_ERREXCEPT` support yet.

## Exception contract

The exact identity and small-integer paths remain exception-clean:

- `pow(x, 0)` returns `1` without adding an exception;
- `pow(1, y)` retains its existing identity behavior;
- exactly representable small integer powers such as `pow(2, 3)`,
  `pow(-2, 3)`, and `pow(2, -3)` do not add floating exceptions;
- existing sticky flags remain sticky across such exact calls;
- successful exact calls preserve an existing `errno` value.

Ordinary noninteger finite composition continues through the existing
`log`/`exp` substrate. Those functions already propagate `FE_INEXACT`, so pow
does not invent a second approximation detector. A representative
`pow(2, 0.5)` therefore preserves `errno` and leaves `FE_INEXACT` set.

Error classes are mapped deliberately:

- a finite negative base with a noninteger exponent returns the existing quiet
  NaN result, sets `errno = EDOM`, and raises `FE_INVALID`;
- zero raised to a negative exponent returns the existing signed infinity,
  sets `errno = ERANGE`, and raises `FE_DIVBYZERO`;
- finite overflow sets `errno = ERANGE` and raises
  `FE_OVERFLOW | FE_INEXACT`;
- finite underflow into the subnormal/zero result class sets `errno = ERANGE`
  and raises `FE_UNDERFLOW | FE_INEXACT`.

`powf` reuses the binary64 implementation and then narrows to binary32. If that
narrowing itself creates a binary32 overflow or underflow while the finite input
classification remains within the existing powf range contract, powf adds the
corresponding `FE_OVERFLOW`/`FE_UNDERFLOW` plus `FE_INEXACT` and preserves the
existing `ERANGE` behavior.

The implementation raises public flags through `feraiseexcept`; it does not
maintain a pow-private software exception state.

## Numerical boundary

This phase does not change integer-exponent classification, exponentiation by
squaring, the negative-base sign rules, the `exp`/`log` composition, or the
public special-value matrix. The existing `pow_probe` and controlled
`pow_differential` suite remain the numerical contract.

No correctly-rounded claim is added. The new tests cover only the floating-
environment side effects of result classes that were already part of the
executable pow baseline.

## Executable evidence

The freestanding `fenv_pow_probe` verifies:

- clean exact positive, negative, and reciprocal integer powers;
- ordinary noninteger `FE_INEXACT` propagation;
- `EDOM + FE_INVALID` for a negative-base noninteger domain failure;
- `ERANGE + FE_DIVBYZERO` for positive and negative signed-zero poles;
- `ERANGE + FE_OVERFLOW + FE_INEXACT` for binary64 overflow;
- `ERANGE + FE_UNDERFLOW + FE_INEXACT` for binary64 underflow;
- binary32 narrowing overflow and underflow through `powf`;
- preservation of a pre-existing sticky flag across an exact result.

The hosted `fenv_pow_interop` compiles the production pow family under renamed
symbols and connects it to the renamed exp/log and fenv implementations plus the
real x86-64 fenv assembly primitive. Host `<fenv.h>` observes the resulting
x87/MXCSR flags, proving that the side effects are hardware-visible.

Pinned tiny-c compiles the production pow source together with the normal
mini-libc source set and runs exact, inexact, domain, pole, and range cases in
`tiny_fenv_integration.c`. The same executable is linked and run through GNU
`ld` and the pinned mini-elf-toolchain.

## Phase boundary and promotion

The pow exception phase is complete. More exponent examples for the same
result classes would now be low-value repetition.

The next exception-propagation frontier should be chosen by semantic coverage,
not API count. The strongest remaining candidates are domain/range-heavy
families whose result/errno matrices already exist but whose floating exception
side effects are still unclaimed: square-root and inverse-domain functions such
as `sqrt`, `asin`, `acos`, and `atanh`, followed by the remaining transcendental
and special-function families.

A follow-on slice should preserve each family's current numerical and errno
contract, add deterministic `FE_INVALID`, `FE_DIVBYZERO`, range, and ordinary
inexact behavior where applicable, and prove the effects through freestanding,
host-fenv, pinned tiny-c, and mini-elf execution.

`math_errhandling` must remain `MATH_ERRNO` until exception coverage is broad
enough that advertising `MATH_ERREXCEPT` would describe the library as a whole
rather than a small set of promoted families.
