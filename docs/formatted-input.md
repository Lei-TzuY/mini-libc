# Formatted input ABI and phase status

The formatted-input engine uses one private parser over a tagged scanner-source
abstraction. FILE-backed scanning serves that source through the same buffered
`fgetc`/`ungetc` cursor used by `fread` and `fgets`; memory-backed scanning serves
it directly from a NUL-terminated byte string. The parser, conversion rules,
matching behavior, rollback policy, and destination cursor are shared across
ordinary variadic and public `va_list` entry points. `sscanf`/`vsscanf` therefore
do not fabricate a `FILE`, open a descriptor, issue a raw read, or fork a second
parser.

## Public surface

`<stdio.h>` exposes:

```c
int scanf(const char *restrict format, ...);
int fscanf(FILE *restrict stream, const char *restrict format, ...);
int sscanf(const char *restrict s, const char *restrict format, ...);
int vscanf(const char *restrict format, va_list ap);
int vfscanf(FILE *restrict stream, const char *restrict format, va_list ap);
int vsscanf(const char *restrict s, const char *restrict format, va_list ap);
```

The executable scanner supports `%d`, `%i`, `%u`, `%o`, `%x`, `%X`, `%c`, `%s`,
`%[...]`, `%f`, `%F`, `%e`, `%E`, `%g`, `%G`, and `%%`. Integer assignments
support the `hh`, `h`, `l`, and `ll` length modifiers. Floating assignments use
no length modifier for `float *` and `l` for `double *`; `h`, `hh`, and `ll` are
rejected for floating conversions. Conversions may use a positive decimal field
width and, except for `%%`, the `*` assignment-suppression modifier.

`%d`, `%i`, `%u`, `%o`, `%x`, `%X`, `%s`, and all currently supported floating
conversions skip leading C whitespace. `%c` and `%[` do not. Whitespace in the
format consumes any amount of C whitespace from the input. A literal format byte
must match the next input byte.

## Integer and scanset model

Integer scanning shares one base-aware parser. `%d` and `%u` use decimal, `%o`
uses octal, `%x`/`%X` use hexadecimal with an optional `0x`/`0X` prefix, and
`%i` selects hexadecimal after `0x`/`0X`, octal after a leading zero, and
decimal otherwise. The field width includes an optional sign and any base
prefix. The scanner follows the input-item rule rather than rolling incomplete
prefixes back: for example, `%2i` applied to `0x9` consumes the width-limited
`0x`, reports a matching failure because that item cannot be converted, and
leaves `9` logically unread within the active source.

Scansets match a non-empty byte sequence and append a null terminator for a
non-suppressed assignment. A leading `^` negates the set; `]` is a literal member
when it is the first set byte after `[` or `[^`. mini-libc gives `-` a
deterministic implementation-defined policy: an interior ascending `a-z` form
is a range, while `-` outside such a range is literal. Field width limits matched
bytes, and the first byte outside the set remains unread.

## Floating conversion model

The first floating-input slice is deliberately a bounded **finite decimal and
scientific** parser shared by `%f`, `%F`, `%e`, `%E`, `%g`, and `%G`. These
conversion letters currently select the same accepted lexical form:

- one optional `+` or `-`;
- decimal digits with at most one `.` and at least one digit overall;
- an optional `e` or `E`, followed by an optional sign and at least one decimal
  exponent digit.

Field width limits the entire input item, including sign, decimal point, exponent
marker, and exponent sign. Assignment suppression still performs lexical
consumption but deliberately skips numeric range conversion and destination
access. A suppressed huge exponent can therefore be consumed without inventing
an `ERANGE` side effect for a value that is never assigned.

The converter retains at most 18 significant decimal digits, tracks omitted
fractional and excess digits as a decimal exponent adjustment, and rounds a
discarded tail to nearest with ties to even before binary64 scaling. Leading
zeros do not consume the 18-digit significant budget. This is a bounded,
correctness-focused conversion contract, not a claim that mini-libc already has
a complete correctly-rounded general-purpose decimal-to-binary implementation.

A nonzero finite value that overflows binary64, underflows all the way to zero,
overflows `float` when the destination is `float *`, or narrows from nonzero
double to zero float reports `ERANGE`, performs no assignment for that
conversion, and stops scanning under the same deterministic range-failure policy
used by integer input. Representable successful conversions preserve the prior
`errno`. Exact zero is accepted, and an input negative sign is retained for
negative zero; executable coverage scans `-0.0` and then sends the resulting
double through the existing `%f` output path to prove the sign survives the
cross-layer round trip.

Incomplete exponent syntax follows the scanner's input-item policy rather than
pretending the exponent marker was never consumed. For example, scanning
`1e+X` consumes `1e+`, reports a matching failure, and leaves `X` as the one
looked-ahead byte for the next operation. The real FILE regression verifies that
a following `fgetc` receives that `X`.

This phase intentionally does **not** admit `inf`, `infinity`, `nan`, hexadecimal
floating input (`%a`/`%A`), `long double`, or locale-dependent decimal syntax.
It also does not expose the scanner-local conversion machinery as `strtof` or
`strtod`; a public standard conversion API will require its broader lexical,
`endptr`, and range contract to be implemented first.

## Source and failure model

The parser consumes at most one byte beyond a completed input item. FILE sources
restore that byte through the guaranteed one-byte `ungetc` path. String sources
restore only the immediately preceding byte by moving the private memory cursor
back one position. This keeps the same one-byte logical rollback invariant across
all six public entry points without expanding FILE pushback state.

A string source treats its terminating NUL as input exhaustion and never exposes
that terminator to a conversion. It performs no allocation and no syscall. The
hosted scanner regression locks this isolation by asserting that memory-backed
integer and floating scanning leave the fake raw-read call counter unchanged even
when FILE-backed scanner data is already buffered in `stdin`.

The return value is the number of successful non-suppressed assignments. A
matching failure returns the number already assigned, including zero. An input
failure or EOF before the first receiving conversion is assigned returns `EOF`;
a later input failure returns the assignment count already completed. FILE EOF
and error indicators continue to come from the shared FILE read machinery;
`sscanf`/`vsscanf` have no FILE indicator state to modify.

The scanner preserves an existing `errno` on successful representable input.
Integer and floating range failures use the deterministic `ERANGE`/no-assignment
policy described above. Invalid FILE streams, null scanner inputs/formats, or
unsupported/invalid format surfaces use the existing deterministic `EINVAL`
policy.

## Variadic call boundary

The scanner core consumes one private `mini_scan_args` cursor containing up to
five remaining 8-byte GP-register words, an overflow-stack pointer, a register
index, and a register count. Every receiving argument, including `float *` and
`double *`, is a pointer and therefore INTEGER-class under the current x86-64
SysV contract.

Ordinary variadic assembly entries populate that cursor directly:

- `scanf(format, ...)` captures `rsi`, `rdx`, `rcx`, `r8`, and `r9`, then the
  overflow-stack cursor;
- `fscanf(stream, format, ...)` captures `rdx`, `rcx`, `r8`, and `r9`, then the
  overflow-stack cursor;
- `sscanf(string, format, ...)` has the same named-argument shape as `fscanf`.

Public `vscanf`/`vfscanf`/`vsscanf` reuse the public SysV `va_list` contract from
`<stdarg.h>`. Their assembly adapters read `gp_offset`, `overflow_arg_area`, and
`reg_save_area`, copy the still-available GP slots into `mini_scan_args`, carry
the overflow pointer forward unchanged, and then enter the exact same scanner
dispatch used by the ordinary entry points. `vscanf` supplies the inherited
`stdin` object and tail-enters `vfscanf`; `vsscanf` enters the existing string
source dispatch. No scanner C path calls compiler-specific `va_arg` builtins.

`fp_offset` remains part of the public `va_list` state but is not consumed by the
scanner. Floating formatted input changes the type of the object pointed to, not
the ABI class of the variadic argument itself; the scanner therefore needs no XMM
argument lane for `%f` or `%lf`.

## Executable evidence

The deterministic fake-read harness routes its six-destination stdin integer scan
through a caller-defined `vscanf` wrapper. A following cached-input case uses
`vfscanf(stdin, ...)`, while memory-backed `vsscanf` proves that no fake raw read
occurs. Floating memory coverage adds ordinary/public-`va_list` decimal and
scientific conversions, width followed by `%c`, suppression of a huge exponent,
and double/float overflow and underflow cases with unchanged destinations.

The freestanding scanner probe covers the established integer/string/scanset
surface plus owned-file `vfscanf` floating input, memory-backed `vsscanf`, and
stdin `vscanf`. It verifies `float *` and `double *`, decimal-point and exponent
forms, errno preservation, malformed-exponent one-byte lookahead, and a signed
zero scan-to-format round trip through the existing output formatter.

The pinned tiny-c integration compiles caller-defined wrappers for all three
public variadic scanner APIs. It now compiles and executes floating FILE, memory,
and stdin destinations itself, then routes scanned negative zero back through
`snprintf`. The same executable runs through GNU `ld` and the pinned
`mini-elf-toolchain`. Repository CI also compiles and runs the full
freestanding/runtime suite under GCC and Clang and verifies host-libc
independence.

## Phase boundary and next frontier

FILE-backed and memory-backed formatted input now share integer, character,
scanset, and bounded finite floating conversions across ordinary variadic and
public `va_list` entry points. Floating input therefore extends the existing
parser/source architecture rather than creating a parallel scanner or requiring
an unnecessary XMM destination ABI.

The next higher architectural frontier is a **public decimal floating conversion
core** for `strtof`/`strtod`. That phase should refactor the proven scanner-local
finite decimal machinery into a reusable conversion engine only when the public
contract is real: `endptr`/no-conversion semantics, range behavior, `inf`/`nan`,
and the standard hexadecimal floating lexical surface must be addressed rather
than exposing today's bounded scanner subset under a broader API name. Once that
core exists, scanner `%a`/`%A` and richer floating input can reuse it instead of
duplicating numeric parsing.

`long double`, wide-character scanning, locale-sensitive behavior, `%n`, pointer
formatting, configurable buffering, `tmpfile`, threading/TLS, C11
exclusive-create modes, and allocator tuning remain separate later phases.
