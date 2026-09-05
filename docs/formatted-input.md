# Formatted input ABI and phase status

The formatted-input engine uses one private parser over a tagged scanner-source
abstraction. FILE-backed scanning serves that source through the same buffered
`fgetc`/`ungetc` cursor used by `fread` and `fgets`; memory-backed scanning serves
it directly from a NUL-terminated byte string. The parser, matching behavior,
rollback policy, and destination cursor are shared across ordinary variadic and
public `va_list` entry points. `sscanf`/`vsscanf` therefore do not fabricate a
`FILE`, open a descriptor, issue a raw read, or fork a second scanner.

Floating numeric lexing/conversion is now delegated to the stdio-independent
engine documented in `docs/floating-conversion.md`. `strtof`/`strtod` and the
scanner share that engine while selecting distinct transaction policies where
their standards-facing semantics differ.

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
`%[...]`, `%f`, `%F`, `%e`, `%E`, `%g`, `%G`, `%a`, `%A`, and `%%`. Integer
assignments support the `hh`, `h`, `l`, and `ll` length modifiers. Floating
assignments use no length modifier for `float *` and `l` for `double *`; `h`,
`hh`, and `ll` are rejected for floating conversions. Conversions may use a
positive decimal field width and, except for `%%`, the `*`
assignment-suppression modifier.

`%d`, `%i`, `%u`, `%o`, `%x`, `%X`, `%s`, and every floating conversion skip
leading C whitespace. `%c` and `%[` do not. Whitespace in the format consumes
any amount of C whitespace from the input. A literal format byte must match the
next input byte.

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

All floating conversion letters enter one shared lexical/conversion engine. The
accepted C-locale input families are:

- decimal digits with an optional point and optional `e`/`E` exponent;
- `0x`/`0X` hexadecimal input with an optional point and optional `p`/`P`
  exponent;
- case-insensitive `INF`/`INFINITY`;
- case-insensitive `NAN`/`NAN(payload)` with an ASCII alphanumeric/underscore
  payload.

One optional sign may precede every family. `%a`/`%A` are executable rather than
rejected syntax; `%f`, `%e`, and `%g` families intentionally select the same
shared input grammar rather than separate notation-specific parsers.

Field width limits the complete input item. Assignment suppression performs
lexical consumption with numeric conversion disabled, so a suppressed huge
exponent does not invent an `ERANGE` side effect for a value that will never be
stored.

The shared converter retains at most 18 significant decimal digits or 15
significant hexadecimal digits and rounds discarded tails to nearest with ties
to even before bounded binary scaling. This is an explicit implementation bound,
not a claim of a general correctly-rounded decimal-to-binary algorithm. See
`docs/floating-conversion.md` for the reusable numeric contract and differential
evidence boundary.

A range status stops scanning and performs no receiving assignment. That includes
binary64 overflow/underflow under the shared engine's deterministic range policy
and binary32 overflow/subnormal/underflow detected while narrowing a no-length
conversion to `float`. Exact zero is accepted without a range error, and negative
zero retains its sign.

The scanner deliberately keeps its input-item commit policy even though public
`strtof`/`strtod` use transactional rollback for incomplete optional suffixes.
Scanning `1e+X` consumes `1e+`, reports a matching failure, and leaves `X` as the
one looked-ahead byte. By contrast, `strtod("1e+X", &end)` converts `1` and leaves
`end` at the `e`. The two behaviors share lexical/conversion code but not caller
transaction semantics.

## Source and failure model

The scanner consumes at most one byte beyond a completed input item. FILE sources
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
Integer and floating range failures use deterministic `ERANGE` with no assignment.
Invalid FILE streams, null scanner inputs/formats, or unsupported/invalid format
surfaces use the existing deterministic `EINVAL` policy.

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

`fp_offset` remains part of public `va_list` state but is not consumed by the
scanner. Floating input changes the type of the pointed-to object, not the ABI
class of the variadic argument itself, so `%f`, `%lf`, and `%la` require no XMM
argument lane.

## Executable evidence

The deterministic fake-read harness retains cached stdin, FILE, string-source,
width, suppression, range, malformed exponent, and errno regressions after the
floating refactor. This is important because the scanner no longer owns its own
decimal scaling implementation.

The freestanding scanner probe continues to cover owned-file `vfscanf`,
memory-backed `vsscanf`, stdin `vscanf`, decimal/scientific floating input,
negative zero, malformed-exponent lookahead, and the established integer/string/
scanset surface. `strtod_probe` adds `%la`, infinity, and NaN scanner execution
while simultaneously exercising public `strtof`/`strtod`, proving both callers
converge on the shared engine.

The pinned tiny-c integration compiles and executes public hexadecimal `strtod`,
decimal `strtof`, and `%la` via a caller-defined `vsscanf` wrapper. Existing FILE,
memory, and stdin floating destinations remain covered. The same executable is
linked and run through GNU `ld` and the pinned mini-elf toolchain. GCC and Clang
run the complete freestanding/runtime suite and host-libc-independence inspection.

## Phase boundary and next frontier

Formatted input and public floating conversion are now closed companion phases:
FILE/string source handling remains in stdio, while decimal/hex/special lexical
and numeric conversion is owned by one reusable internal engine. The formatter
has now caught up as well: `%f/%F`, `%e/%E`, `%g/%G`, and `%a/%A` output execute
through the established XMM/public-`va_list` and FILE/memory architecture rather
than requiring another floating parser or transport layer.

The next architectural promotion is **configurable stream buffering and buffer
ownership**. That work belongs below formatted input/output: `setvbuf`/`setbuf`
should select full, line, or unbuffered behavior while preserving this scanner's
shared logical read cursor, one-byte pushback, update-stream direction barriers,
positioning semantics, and error indicators. Caller-provided versus libc-owned
buffer lifetime must be explicit and executable before higher-level stdio grows
further.

`long double`, wide-character scanning, locale-sensitive behavior, `%n`, pointer
formatting, `tmpfile`, threading/TLS, C11 exclusive-create modes, and allocator
tuning remain separate later phases.