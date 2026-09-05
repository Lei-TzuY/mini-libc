# Formatted input ABI and phase status

The formatted-input engine is built on the same private buffered `FILE` cursor
used by `fgetc`, `fread`, `fgets`, and `ungetc`. It does not bypass stream
buffering or issue raw reads directly.

## Public surface

`<stdio.h>` exposes:

```c
int scanf(const char *restrict format, ...);
int fscanf(FILE *restrict stream, const char *restrict format, ...);
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
leaves `9` logically unread.

Scansets match a non-empty byte sequence and append a null terminator for a
non-suppressed assignment. A leading `^` negates the set; `]` is a literal member
when it is the first set byte after `[` or `[^`. mini-libc gives `-` a
deterministic implementation-defined policy: an interior ascending `a-z` form
is a range, while `-` outside such a range is literal. Field width limits matched
bytes, and the first byte outside the set remains unread.

The scanner consumes at most one byte beyond a completed input item and restores
that byte with the guaranteed one-byte `ungetc` path, so this milestone does not
expand the FILE pushback model.

The return value is the number of successful non-suppressed assignments. A
matching failure returns the number already assigned, including zero. An input
failure or EOF before the first receiving conversion is assigned returns `EOF`;
a later input failure returns the assignment count already completed. Sticky
stream EOF/error state comes from the shared `FILE` read machinery.

The scanner preserves an existing `errno` on successful representable input.
For this correctness-focused implementation, an integer magnitude that the
selected destination type cannot represent is detected without signed-overflow
undefined behavior, reports `ERANGE`, performs no assignment for that conversion,
and stops scanning. Invalid stream or unsupported/invalid format surfaces use
the existing deterministic `EINVAL` policy.

## Variadic call boundary

Production C remains free of compiler-specific variadic builtins. Small SysV
AMD64 assembly entries capture INTEGER-class variadic arguments into the same
kind of fixed private cursor used by formatted output:

- `scanf(format, ...)` captures `rsi`, `rdx`, `rcx`, `r8`, and `r9`, then the
  overflow-stack cursor.
- `fscanf(stream, format, ...)` captures `rdx`, `rcx`, `r8`, and `r9`, then the
  overflow-stack cursor.

All supported receiving arguments are pointers, so this phase needs no XMM
variadic save area. The pinned tiny-c integration deliberately places the fifth
`fscanf` destination pointer on the overflow stack while parsing `%i`, decimal,
scansets, and `%x`, and executes the same call through both GNU `ld` and
`mini-elf-toolchain`.

## Executable evidence

The freestanding scanner probe covers owned-file `fscanf`, stdin `scanf`, all
four integer destination lengths, decimal/octal/hexadecimal/auto-base input,
optional hexadecimal prefixes, string/character input, literal `%`, field width,
suppression, positive/negated/ranged scansets, literal `]` and `-` scanset
members, matching failure with an unread delimiter byte, a width-truncated `0x`
input item, EOF return semantics, and errno preservation.

A deterministic fake-read harness proves that the scanner-breadth paths still
reuse one 256-byte FILE refill, that a sixth `scanf` destination crosses from GP
registers to the overflow stack in the existing core coverage, that scanset
matching and mismatch operate on cached input, and that sticky EOF suppresses
redundant raw reads.

The repository CI compiles every production C source with GCC, Clang, and the
pinned `tiny-c-compiler`, links the scanner assembly entry into the mini-libc
archive, and runs the integration executable through both GNU `ld` and the pinned
`mini-elf-toolchain` with host-libc-independence checks.

## Phase boundary and next frontier

The formatted-input FILE scanner breadth is now part of the executable baseline:
base-aware integer input and scansets no longer belong to the roadmap.

The next higher-value architectural frontier is **memory-backed formatted
input**, starting with `sscanf` without duplicating the parser. That requires an
internal scanner-source abstraction capable of serving the same get/unget
contract from either a buffered `FILE` or a bounded in-memory string, followed by
a compiler-neutral variadic `sscanf` entry and differential/executable evidence.
The goal is to generalize the scanner architecture rather than fake a FILE
descriptor or fork a second parser.

Floating-point input, `%n`, wide-character scanning, locale-sensitive behavior,
public `stdarg.h`/`vfscanf`, `snprintf`/memory-backed output, configurable
buffering, `tmpfile`, threading/TLS, and C11 exclusive-create modes remain
separate later phases.
