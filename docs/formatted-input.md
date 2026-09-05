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
all six public entry points without expanding FILE pushback state.

A string source treats its terminating NUL as input exhaustion and never exposes
that terminator to a conversion. It performs no allocation and no syscall. The
hosted scanner regression locks this isolation by asserting that memory-backed
scanning leaves the fake raw-read call counter unchanged even when FILE-backed
scanner data is already buffered in `stdin`.

The return value is the number of successful non-suppressed assignments. A
matching failure returns the number already assigned, including zero. An input
failure or EOF before the first receiving conversion is assigned returns `EOF`;
a later input failure returns the assignment count already completed. FILE EOF
and error indicators continue to come from the shared FILE read machinery;
`sscanf`/`vsscanf` have no FILE indicator state to modify.

The scanner preserves an existing `errno` on successful representable input.
For this correctness-focused implementation, an integer magnitude that the
selected destination type cannot represent is detected without signed-overflow
undefined behavior, reports `ERANGE`, performs no assignment for that conversion,
and stops scanning. Invalid FILE streams, null scanner inputs/formats, or
unsupported/invalid format surfaces use the existing deterministic `EINVAL`
policy.

## Variadic call boundary

The scanner core consumes one private `mini_scan_args` cursor containing up to
five remaining 8-byte GP-register words, an overflow-stack pointer, a register
index, and a register count. Every supported receiving argument is a pointer and
therefore INTEGER-class under the current x86-64 SysV contract.

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
current scanner because no supported conversion receives floating-point values.
This phase makes no floating-point, vector, long-double, or aggregate variadic
claim.

## Executable evidence

The deterministic fake-read harness routes its six-destination stdin scan through
a caller-defined `vscanf` wrapper. With one named GP argument in the wrapper, the
sixth destination pointer is stack-resident. A following cached-input case uses
`vfscanf(stdin, ...)`, proving the public FILE variadic path does not disturb the
single-refill buffered cursor. The memory case uses `vsscanf` and still proves
that no fake raw read occurs.

The freestanding scanner probe covers owned-file `vfscanf`, stdin `vscanf`, and
memory-backed `vsscanf`; all four integer destination lengths; decimal/octal/
hexadecimal/auto-base input; optional hexadecimal prefixes; string/character
input; literal `%`; field width; suppression; positive/negated/ranged scansets;
literal `]` and `-` scanset members; matching failure; width-truncated `0x`;
EOF return semantics; and errno preservation. Its first `vfscanf` call receives
eight destination pointers, forcing several assignments through
`overflow_arg_area`.

The pinned tiny-c integration compiles caller-defined wrappers for all three
public variadic scanner APIs. `vfscanf` reuses the existing positioned file scan;
`vsscanf` reuses the existing memory-source scan; `vscanf` receives six integers
from actual process stdin, again forcing the sixth destination across the SysV GP
register boundary. The same integration executable is linked and run through
GNU `ld` and the pinned `mini-elf-toolchain`. Repository CI also compiles and runs
the freestanding/runtime suite under GCC and Clang and verifies host-libc
independence.

## Phase boundary and next frontier

FILE-backed and memory-backed formatted input now have both ordinary variadic and
public `va_list` entry points over one parser and one conversion model. The input
side of the public variadic core is therefore closed: new formatted-input
features must extend the shared parser/source architecture rather than create a
parallel `v*` implementation.

The next higher architectural frontier is **floating-point variadic transport
and formatted floating output**. The current private argument cursor is
INTEGER-class-only even though public SysV `va_list` already carries `fp_offset`.
A coherent next slice should extend the variadic transport with deterministic XMM
register-save handling and then admit a first real floating conversion into the
shared formatter, with GCC/Clang/pinned tiny-c/mini-elf executable evidence. It
must not claim floating-point support from ABI plumbing alone.

Floating-point input, `%n`, pointer formatting, wide-character scanning,
locale-sensitive behavior, configurable buffering, `tmpfile`, threading/TLS,
C11 exclusive-create modes, and allocator tuning remain separate later phases.
