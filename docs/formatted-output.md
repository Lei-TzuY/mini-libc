# Formatted output ABI and phase status

The formatted-output engine uses one private parser/conversion core and one
private tagged sink abstraction. `printf` and `fprintf` select the existing
buffered `FILE` sink; `snprintf` selects a bounded memory sink. No second
string-only formatter, fake `FILE`, heap staging buffer, or raw-write shortcut is
used.

## Public surface

`<stdio.h>` exposes:

```c
int printf(const char *restrict format, ...);
int fprintf(FILE *restrict stream, const char *restrict format, ...);
int snprintf(char *restrict s, size_t n, const char *restrict format, ...);
```

All three entry points share the same executable conversion surface: `%d`, `%i`,
`%u`, `%o`, `%x`, `%X`, `%c`, `%s`, and `%%`; `-`, `+`, space, `#`, and `0`
flags; fixed or `*` width; fixed or `*` precision; and `hh`, `h`, `l`, and `ll`
integer length modifiers. Existing deterministic invalid-format and return-count
overflow behavior remains unchanged.

## Sink model

The private formatter sink has two modes:

- the FILE sink forwards emitted bytes to `__mini_stdio_write`, preserving the
  existing buffered stream/error behavior used by `printf` and `fprintf`;
- the memory sink stores at most `n - 1` bytes when `n > 0`, never calls the FILE
  output path, and keeps the stored prefix null-terminated after every emit.

The formatter tracks the logical output count independently from the number of
bytes actually stored in the bounded memory sink. Truncation therefore does not
stop parsing or counting. On successful formatting, `snprintf` returns the full
number of bytes that would have been produced with sufficient space, excluding
the terminating null byte.

For `n > 0`, the destination is null-terminated even when truncation occurs. For
`n == 0`, the destination is never dereferenced and may be null. Passing a null
destination with `n > 0` is a deterministic mini-libc extension that returns
`EOF` and reports `EINVAL`.

As with the existing FILE formatter, a logical result that cannot fit in the
positive `int` return range is rejected deterministically with `EINVAL` rather
than silently wrapping the return count.

## Variadic call boundary

Production C remains free of compiler-specific variadic builtins. Small SysV
AMD64 assembly entries capture INTEGER-class variadic arguments into the same
private cursor consumed by the shared formatter:

- `printf(format, ...)` captures `rsi`, `rdx`, `rcx`, `r8`, and `r9`, then the
  overflow-stack cursor;
- `fprintf(stream, format, ...)` captures `rdx`, `rcx`, `r8`, and `r9`, then the
  overflow-stack cursor;
- `snprintf(s, n, format, ...)` captures `rcx`, `r8`, and `r9`, then the
  overflow-stack cursor.

The freestanding and pinned tiny-c integration deliberately format five
`snprintf` values, so the fourth and fifth variadic values are fetched from the
overflow stack rather than only exercising register-resident arguments.

## Executable evidence

The freestanding stdio probe covers full-buffer memory formatting, five-argument
register/stack traversal, truncation with guard bytes beyond the declared bound,
`n == 1`, `n == 0`, `snprintf(NULL, 0, ...)`, null-with-positive-size rejection,
errno preservation, and successful memory formatting after stdout's underlying
file descriptor has been closed. The latter demonstrates that the memory sink is
independent of FILE output and raw stream writes.

The pinned tiny-c integration executes both a complete five-value `snprintf` and
a truncated `snprintf`, then runs the same binary through GNU `ld` and the pinned
`mini-elf-toolchain`. Repository CI also compiles production C with GCC and
Clang, runs the normal runtime/probe suite, and verifies host-libc independence.

## Phase boundary and next frontier

Memory-backed formatted output is now part of the executable baseline. The
formatter is no longer a FILE-only subsystem: FILE and bounded-memory output
share one parser and conversion engine, just as FILE and memory input now share
one scanner core.

The next higher-value architectural frontier is a **public variadic core**:
introduce an ABI-defined `stdarg.h`/`va_list` contract that can be consumed by
mini-libc itself and by callers, then expose `vprintf`/`vfprintf`/`vsnprintf` and
the corresponding scanner-side `vscanf`/`vfscanf`/`vsscanf` surfaces without
forking either parser. That phase must preserve compiler neutrality across GCC,
Clang, the pinned tiny-c compiler, and the mini-elf integration path; it should
not be implemented as compiler-specific builtin wrappers that bypass the current
ABI evidence.

Floating-point formatting/scanning, `%n`, wide-character I/O, locale-sensitive
behavior, configurable buffering, `tmpfile`, threading/TLS, and C11
exclusive-create modes remain separate later phases.
