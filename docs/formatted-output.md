# Formatted output ABI and phase status

The formatted-output engine uses one private parser/conversion core and one
private tagged sink abstraction. `printf` and `fprintf` select the existing
buffered `FILE` sink; `snprintf` selects a bounded memory sink. The public
`vprintf`, `vfprintf`, and `vsnprintf` entries feed caller-created `va_list`
state into those same paths. No second formatter, fake `FILE`, heap staging
buffer, or raw-write shortcut is used.

## Public surface

`<stdio.h>` exposes:

```c
int printf(const char *restrict format, ...);
int fprintf(FILE *restrict stream, const char *restrict format, ...);
int snprintf(char *restrict s, size_t n, const char *restrict format, ...);
int vprintf(const char *restrict format, va_list ap);
int vfprintf(FILE *restrict stream, const char *restrict format, va_list ap);
int vsnprintf(char *restrict s, size_t n,
              const char *restrict format, va_list ap);
```

All six entry points share the same executable conversion surface: `%d`, `%i`,
`%u`, `%o`, `%x`, `%X`, `%c`, `%s`, and `%%`; `-`, `+`, space, `#`, and `0`
flags; fixed or `*` width; fixed or `*` precision; and `hh`, `h`, `l`, and `ll`
integer length modifiers. Existing deterministic invalid-format and return-count
overflow behavior remains unchanged.

## Sink model

The private formatter sink has two modes:

- the FILE sink forwards emitted bytes to `__mini_stdio_write`, preserving the
  existing buffered stream/error behavior used by `printf`, `fprintf`,
  `vprintf`, and `vfprintf`;
- the memory sink stores at most `n - 1` bytes when `n > 0`, never calls the FILE
  output path, and keeps the stored prefix null-terminated after every emit for
  `snprintf` and `vsnprintf`.

The formatter tracks the logical output count independently from the number of
bytes actually stored in the bounded memory sink. Truncation therefore does not
stop parsing or counting. On successful formatting, `snprintf` and `vsnprintf`
return the full number of bytes that would have been produced with sufficient
space, excluding the terminating null byte.

For `n > 0`, the destination is null-terminated even when truncation occurs. For
`n == 0`, the destination is never dereferenced and may be null. Passing a null
destination with `n > 0` is a deterministic mini-libc extension that returns
`EOF` and reports `EINVAL`.

As with the FILE formatter, a logical result that cannot fit in the positive
`int` return range is rejected deterministically with `EINVAL` rather than
silently wrapping the return count.

## Variadic call boundary

The ordinary variadic entries remain small SysV AMD64 assembly shims that capture
INTEGER-class variadic values into the private formatter cursor:

- `printf(format, ...)` captures `rsi`, `rdx`, `rcx`, `r8`, and `r9`, then the
  overflow-stack cursor;
- `fprintf(stream, format, ...)` captures `rdx`, `rcx`, `r8`, and `r9`, then the
  overflow-stack cursor;
- `snprintf(s, n, format, ...)` captures `rcx`, `r8`, and `r9`, then the
  overflow-stack cursor.

The public `v*` entries consume the SysV `va_list` state instead. They copy the
remaining GP words from `reg_save_area + gp_offset` into the same private cursor
and forward `overflow_arg_area` unchanged before entering the existing formatter.
The parser and sink layers therefore do not know whether their arguments came
from an ordinary `...` entry or a caller-created `va_list`.

The exact `<stdarg.h>` layout, scoped compiler-primitive policy, `va_copy`
contract, and GP-register/overflow-stack evidence are specified in
`docs/variadic-abi.md`.

## Executable evidence

The freestanding stdio probe covers full-buffer memory formatting, five-argument
register/stack traversal, truncation with guard bytes beyond the declared bound,
`n == 1`, `n == 0`, `snprintf(NULL, 0, ...)`, null-with-positive-size rejection,
errno preservation, and successful memory formatting after stdout's underlying
file descriptor has been closed. It additionally compiles caller-defined
variadic wrappers and exercises `vprintf`, `vfprintf`, `vsnprintf`, `va_arg`, and
`va_copy` across GP-register and overflow-stack boundaries.

The pinned tiny-c integration executes complete and truncated memory formatting,
caller-created `va_list` state, all three output-side `v*` APIs, and the same
binary through GNU `ld` and the pinned `mini-elf-toolchain`. Repository CI also
compiles production C with GCC and Clang, runs the normal runtime/probe suite,
and verifies host-libc independence.

## Phase boundary and next frontier

Memory-backed formatted output and the **output side of the public variadic
core** are now executable baseline capabilities. FILE and bounded-memory output
share one parser/conversion engine, while ordinary variadic calls and public
`va_list` calls converge on the same private argument cursor.

The next higher-value architectural frontier is the **input-side public variadic
core**: expose `vscanf`, `vfscanf`, and `vsscanf`, normalize the public SysV
`va_list` representation into the existing scanner destination cursor, and keep
FILE/memory source handling and matching/input-failure semantics unchanged. It
must retain GCC/Clang/tiny-c compiler coverage and pinned mini-elf execution
without forking the scanner parser.

Floating-point formatting/scanning, `%n`, wide-character I/O, locale-sensitive
behavior, configurable buffering, `tmpfile`, threading/TLS, and C11
exclusive-create modes remain separate later phases.
