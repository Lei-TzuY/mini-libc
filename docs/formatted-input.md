# Formatted input ABI and phase status

The formatted-input engine uses one private parser over a tagged scanner-source
abstraction. FILE-backed scanning serves that source through the same buffered
`fgetc`/`ungetc` cursor used by `fread` and `fgets`; memory-backed scanning serves
it directly from a NUL-terminated byte string. The parser, conversion rules,
matching behavior, rollback policy, and variadic destination cursor are shared.
`sscanf` therefore does not fabricate a `FILE`, open a descriptor, issue a raw
read, or fork a second parser.

## Public surface

`<stdio.h>` exposes:

```c
int scanf(const char *restrict format, ...);
int fscanf(FILE *restrict stream, const char *restrict format, ...);
int sscanf(const char *restrict s, const char *restrict format, ...);
```

The executable scanner supports `%d`, `%i`, `%u`, `%o`, `%x`, `%X`, `%c`, `%s`,
`%[...]`, and `%%`. Integer assignments support the `hh`, `h`, `l`, and `ll`
length modifiers. Conversions may use a positive decimal field width and, except
for `%%`, the `*` assignment-suppression modifier.

`%d`, `%i`, `%u`, `%o`, `%x`, `%X`, and `%s` skip leading C whitespace. `%c`
and `%[` do not. Whitespace in the format consumes any amount of C whitespace
from the input. A literal format byte must match the next input byte.

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

The parser consumes at most one byte beyond a completed input item. FILE sources
restore that byte through the guaranteed one-byte `ungetc` path. String sources
restore only the immediately preceding byte by moving the private memory cursor
back one position. This keeps the same one-byte logical rollback invariant across
all three public entry points without expanding FILE pushback state.

A string source treats its terminating NUL as input exhaustion and never exposes
that terminator to a conversion. It performs no allocation and no syscall. The
hosted scanner regression locks this isolation by asserting that `sscanf` leaves
the fake raw-read call counter unchanged even when FILE-backed scanner data is
already buffered in `stdin`.

The return value is the number of successful non-suppressed assignments. A
matching failure returns the number already assigned, including zero. An input
failure or EOF before the first receiving conversion is assigned returns `EOF`;
a later input failure returns the assignment count already completed. FILE EOF
and error indicators continue to come from the shared FILE read machinery;
`sscanf` has no FILE indicator state to modify.

The scanner preserves an existing `errno` on successful representable input.
For this correctness-focused implementation, an integer magnitude that the
selected destination type cannot represent is detected without signed-overflow
undefined behavior, reports `ERANGE`, performs no assignment for that conversion,
and stops scanning. Invalid FILE streams, null scanner inputs/formats, or
unsupported/invalid format surfaces use the existing deterministic `EINVAL`
policy.

## Variadic call boundary

Production C remains free of compiler-specific variadic builtins. Small SysV
AMD64 assembly entries capture INTEGER-class variadic arguments into one fixed
private cursor shared by the scanner core:

- `scanf(format, ...)` captures `rsi`, `rdx`, `rcx`, `r8`, and `r9`, then the
  overflow-stack cursor.
- `fscanf(stream, format, ...)` captures `rdx`, `rcx`, `r8`, and `r9`, then the
  overflow-stack cursor.
- `sscanf(string, format, ...)` has the same named-argument shape as `fscanf` and
  therefore captures `rdx`, `rcx`, `r8`, and `r9`, then the overflow stack.

All supported receiving arguments are pointers, so this phase needs no XMM
variadic save area. The freestanding and pinned tiny-c integrations deliberately
pass five receiving arguments to `sscanf`, placing the fifth destination pointer
on the overflow stack. The same executable is linked and run through both GNU
`ld` and `mini-elf-toolchain`.

## Executable evidence

The freestanding scanner probe covers owned-file `fscanf`, stdin `scanf`, and
memory-backed `sscanf`; all four integer destination lengths; decimal/octal/
hexadecimal/auto-base input; optional hexadecimal prefixes; string/character
input; literal `%`; field width; suppression; positive/negated/ranged scansets;
literal `]` and `-` scanset members; matching failure; width-truncated `0x`;
EOF return semantics; and errno preservation.

The deterministic fake-read harness proves that FILE-backed scanner paths reuse
one 256-byte FILE refill, that a sixth `scanf` destination crosses from GP
registers to the overflow stack, that scanset matching and mismatch operate on
cached input, that sticky EOF suppresses redundant raw reads, and that multiple
`sscanf` calls consume only their memory strings without incrementing the raw
read counter.

The pinned tiny-c integration executes both the existing FILE scanner and a
five-destination `sscanf` call containing auto-base integers, scansets, and
hexadecimal input. Repository CI compiles production C with GCC, Clang, and the
pinned `tiny-c-compiler`, links the common scanner assembly entry into mini-libc,
and runs the integration executable through GNU `ld` and the pinned
`mini-elf-toolchain` with host-libc-independence checks.

## Phase boundary and next frontier

FILE-backed and memory-backed formatted input are now part of the executable
baseline. `scanf`, `fscanf`, and `sscanf` share one parser and one conversion
model; adding a second string-only scanner is explicitly outside the architecture.

The next higher-value symmetric frontier is **memory-backed formatted output**,
starting with bounded `snprintf`. The coherent architectural slice should extract
a private formatter sink abstraction so `printf`/`fprintf` keep using buffered
FILE output while `snprintf` writes into a caller buffer, counts the full would-
have-been-written length, handles size-zero and truncation semantics without
writing out of bounds, and reuses the existing compiler-neutral formatter rather
than cloning it. The executable gate should again include GCC, Clang, pinned
tiny-c, and mini-elf end-to-end execution.

Floating-point input/output, `%n`, wide-character scanning, locale-sensitive
behavior, public `stdarg.h`/`vfscanf`/`vfprintf`, configurable buffering,
`tmpfile`, threading/TLS, and C11 exclusive-create modes remain separate later
phases.
