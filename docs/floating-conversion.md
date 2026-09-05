# Floating conversion core and phase status

mini-libc now has one stdio-independent floating lexical/conversion engine shared
by the public string-conversion APIs and formatted input. The engine is private;
callers provide byte-source callbacks plus an explicit transaction policy rather
than depending on `FILE`, heap storage, or a public parser object.

## Public surface

`<stdlib.h>` exposes:

```c
float strtof(const char *restrict nptr, char **restrict endptr);
double strtod(const char *restrict nptr, char **restrict endptr);
```

Both routines skip the six C whitespace characters and accept one optional sign.
The shared C-locale lexical surface then recognizes:

- decimal input with digits and an optional decimal point, followed by an
  optional `e`/`E` decimal exponent;
- hexadecimal input beginning with `0x`/`0X`, hexadecimal digits with an
  optional point, and an optional `p`/`P` decimal exponent;
- case-insensitive `INF` and `INFINITY`;
- case-insensitive `NAN` and `NAN(payload)`, where the payload contains ASCII
  letters, decimal digits, or `_`.

No-conversion cases return positive zero and, when `endptr` is non-null, store the
original `nptr`. Successful conversions store the first unconsumed byte.
Successful finite or special conversions preserve an existing `errno` unless the
result is in the implementation's explicit range-error class.

The string-conversion policy is transactional around optional continuations. For
example, `strtod("1e+X", &end)` converts `1`, leaves `end` at the `e`, and does
not commit the incomplete exponent. `strtod("0xg", &end)` converts the initial
zero and leaves `end` at the `x`. `INF` remains a valid prefix when an attempted
`INFINITY` suffix is incomplete, and `NAN` remains valid when an attempted
parenthesized payload is incomplete.

## Shared scanner boundary

`scanf`/`fscanf`/`sscanf` and their public `v*` counterparts call the same engine
through adapters over the existing FILE/string scanner sources. `%f`, `%F`,
`%e`, `%E`, `%g`, `%G`, `%a`, and `%A` therefore use the same decimal,
hexadecimal, infinity, NaN, sign, rounding, and range machinery as
`strtof`/`strtod`.

Formatted input deliberately selects a different transaction policy. It preserves
mini-libc's existing input-item contract: once a floating item has begun, an
incomplete exponent is part of the failed input item rather than being rolled
back to the exponent marker. Thus scanning `1e+X` still consumes `1e+`, reports a
matching failure, and leaves `X` as the one looked-ahead byte. This caller policy
is separate from the reusable numeric lexer/converter rather than implemented by
a duplicate scanner parser.

Assignment suppression asks the engine to perform lexical consumption without
numeric range conversion. A suppressed huge exponent therefore does not invent
an `ERANGE` side effect for a destination that is never written.

Scanner receiving arguments remain INTEGER-class pointers under SysV AMD64;
sharing the floating conversion core does not add an XMM argument lane to the
scanner ABI.

## Numeric model and range policy

The conversion core is correctness-focused but intentionally bounded. Decimal
input retains up to 18 significant digits and hexadecimal input retains up to 15
hexadecimal digits. Excess significant digits are represented through an exponent
adjustment and a nearest-even discarded-tail rounding step before binary scaling.
Leading zeroes do not consume the retained significant-digit budget.

This is **not** a claim of a general correctly-rounded decimal-to-binary
implementation. Unrestricted random bit-exact comparison against a mature host
libc exposes legitimate one-ULP differences for some decimal inputs. Repository
differential tests therefore verify the lexical, `endptr`, sign, special-value,
range, and exact/stable numeric contracts that this implementation actually
claims; they do not encode host-specific rounded results merely to make a broad
conformance claim appear green.

For `strtod`, finite overflow returns signed infinity and sets `ERANGE`.
Underflow-to-zero returns signed zero and sets `ERANGE`; a nonzero subnormal
binary64 result is returned and also reports `ERANGE` under mini-libc's
deterministic range policy. Exact zero, including zero with an arbitrarily large
explicit exponent, does not report a range error.

`strtof` reuses the binary64 parse result and then performs an explicit binary32
narrowing check. Binary32 overflow returns the narrowed infinity and reports
`ERANGE`; nonzero subnormal or underflow-to-zero results are returned with
`ERANGE`. Infinity and NaN tokens themselves are accepted without creating a
range error.

Formatted input treats a numeric range status as a conversion failure and does
not write the destination, preserving the scanner contract established for the
first floating-input phase.

Negative zero is preserved through both public conversions and formatted input.
Infinity and NaN are constructed and classified from the target IEEE-754 binary
representations without depending on host-libc conversion routines.

## Executable evidence

The freestanding `strtod_probe` covers decimal and scientific forms, hexadecimal
forms with and without an explicit binary exponent, `INF`/`INFINITY`,
`NAN(payload)`, malformed optional suffix rollback, malformed hexadecimal-prefix
fallback, signed zero, no conversion, `endptr`, errno preservation, double and
float overflow/underflow, and exact-zero large exponents. The same executable
uses `%la` plus infinity/NaN scans to prove formatted input is consuming the
shared engine rather than a second parser.

The hosted differential compares production `strtof`/`strtod` against the host
libc on a controlled corpus whose value comparison is stable and meaningful. It
checks exact result bits where appropriate, NaN classification/sign separately,
`endptr` offsets, and errno behavior. It intentionally does not claim arbitrary
decimal bit-for-bit equivalence.

The pinned tiny-c integration compiles and executes public hexadecimal `strtod`,
decimal `strtof`, and `%la` through the shared scanner. The same integration
binary runs through GNU `ld` and the pinned mini-elf toolchain. GCC and Clang run
the full freestanding/runtime suite plus host-libc-independence inspection.

## Phase boundary and next frontier

Floating lexical/conversion ownership is now centralized: stdlib string
conversion and stdio formatted input share one engine while keeping their
different transaction semantics explicit. The scanner-local floating numeric
subsystem is closed.

The next higher-value floating frontier is **formatted output breadth**. The
output formatter currently proves binary64 transport with bounded `%f`, but
`%e`/`%E`, `%g`/`%G`, and `%a`/`%A` still lack executable conversion behavior.
A coherent next slice should factor reusable binary64 decomposition/rounding and
notation selection into the existing formatter, preserve FILE/memory sinks and
ordinary/public-`va_list` transport, and prove the new families through
GCC/Clang/tiny-c/mini-elf execution without claiming a general-purpose dtoa
algorithm beyond the implemented bounds.

`long double`, locale-sensitive conversion, wide-character I/O, `%n`, pointer
formatting, configurable buffering, `tmpfile`, threading/TLS, C11
exclusive-create modes, and allocator tuning remain separate later phases.
