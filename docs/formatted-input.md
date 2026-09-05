# Formatted input ABI and phase status

The formatted-input core is built on the same private buffered `FILE` cursor used
by `fgetc`, `fread`, `fgets`, and `ungetc`. It does not bypass stream buffering
or issue raw reads directly.

## Public surface

`<stdio.h>` exposes:

```c
int scanf(const char *restrict format, ...);
int fscanf(FILE *restrict stream, const char *restrict format, ...);
```

The first executable scanner slice supports `%d`, `%u`, `%c`, `%s`, and `%%`.
Integer assignments support the `hh`, `h`, `l`, and `ll` length modifiers.
Conversions may use a positive decimal field width and, except for `%%`, the `*`
assignment-suppression modifier.

`%d`, `%u`, and `%s` skip leading C whitespace. `%c` does not. Whitespace in the
format consumes any amount of C whitespace from the input. A literal format byte
must match the next input byte. The scanner consumes at most one byte beyond a
completed input item and restores that byte with the guaranteed one-byte
`ungetc` path, so the first byte that does not belong to an input item remains
logically unread.

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

All supported receiving arguments are pointers, so this slice needs no XMM
variadic save area. The pinned tiny-c integration deliberately places a later
`fscanf` destination pointer on the overflow stack and executes it through both
GNU `ld` and `mini-elf-toolchain`.

## Executable evidence

The freestanding scanner probe covers owned-file `fscanf`, stdin `scanf`, all
four integer destination lengths, string/character input, literal `%`, field
width, suppression, matching failure with an unread delimiter byte, EOF return
semantics, and errno preservation. A deterministic fake-read harness proves that
multiple public `scanf` calls reuse one 256-byte FILE refill, that a sixth scanf
destination crosses from GP registers to the overflow stack, and that sticky EOF
suppresses redundant raw reads.

The repository CI also compiles every production C source with the pinned
`tiny-c-compiler`, links the scanner assembly entry into the mini-libc archive,
and runs the integration executable through both GNU `ld` and the pinned
`mini-elf-toolchain` with host-libc-independence checks.

## Phase boundary and next frontier

This closes the first formatted-input core milestone. The next coherent scanner
frontier is **alternate integer bases plus scansets**: add `%i`, `%o`, `%x`/`%X`
and `%[...]` on the same parser while preserving field-width boundaries,
matching-vs-input failure, one-byte logical rollback, destination-length rules,
and the existing GCC/Clang/tiny-c/mini-elf executable gates.

Floating-point input, `%n`, `sscanf`, wide-character scanning, locale-sensitive
behavior, public `stdarg.h`/`vfscanf`, and configurable buffering remain separate
later phases rather than being mixed into the scanner-breadth milestone.
