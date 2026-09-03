# x86-64 Linux ABI contract

mini-libc currently targets only statically linked x86-64 Linux ELF executables.
This milestone intentionally exposes a raw kernel syscall layer instead of
pretending that POSIX wrappers already exist.

## Process entry

The linker entry point is `_start`, not a host CRT symbol. Linux enters `_start`
with the initial process stack headed by `argc`, followed by `argv[]`, a null
pointer, `envp[]`, another null pointer, and then the ELF auxiliary vector.
`crt0.S` preserves the original stack address, aligns `%rsp` down to a 16-byte
boundary for the System V AMD64 function-call convention, and calls
`__mini_start`.

`__mini_start` decodes `argc`, `argv`, and `envp`, records the original
environment vector through the implementation-only `__mini_set_envp` hook,
invokes `main(int, char **, char **)`, then forwards the returned status to
`mini_sys_exit`. There are currently no constructors, destructors, TLS setup,
`atexit` handlers, or other libc initialization hooks.

## Function ABI

C functions use the System V AMD64 ABI. Integer/pointer arguments begin in
`rdi`, `rsi`, `rdx`, `rcx`, `r8`, and `r9`. A `call` is issued with `%rsp`
16-byte aligned, making the callee entry stack congruent to 8 modulo 16 after
the return address is pushed.

## Linux syscall ABI

The `syscall` instruction uses:

- syscall number: `rax`
- arguments: `rdi`, `rsi`, `rdx`, `r10`, `r8`, `r9`
- return value: `rax`
- clobbers: `rcx`, `r11`, condition codes as defined by the architecture

The six-argument `mmap` wrapper explicitly moves the fourth C argument from
`rcx` to `r10` before executing `syscall`.

Implemented x86-64 Linux syscall numbers are:

| API | Number |
| --- | ---: |
| `mini_sys_read` | 0 |
| `mini_sys_write` | 1 |
| `mini_sys_close` | 3 |
| `mini_sys_lseek` | 8 |
| `mini_sys_mmap` | 9 |
| `mini_sys_munmap` | 11 |
| `mini_sys_brk` | 12 |
| `mini_sys_exit` | 60 |

The wrappers expose the kernel's raw `rax` convention rather than translating it
to POSIX libc results. For the ordinary integer-returning calls, failures remain
negative errno values in `[-4095, -1]`. Raw Linux `brk` is an important
exception: `mini_sys_brk(NULL)` returns the current program break, a successful
request returns the resulting break, and a refused request returns the unchanged
break rather than `-ENOMEM`. No raw wrapper updates libc `errno`.

## errno storage ABI

`<errno.h>` currently exposes the Linux values `EIO = 5`, `ENOMEM = 12`,
`EINVAL = 22`, and `ERANGE = 34` and defines `errno` as a modifiable `int` lvalue backed by
`__mini_errno_location()`. The accessor currently returns one process-global,
zero-initialized BSS slot. This is deliberately not thread-local because
mini-libc has no TLS runtime yet. Keeping access behind the
implementation-reserved accessor allows a later TLS implementation without
changing source code that uses the `errno` macro.

`strtol` and `strtoul` use `EINVAL` for unsupported bases and `ERANGE` for range
overflow. Successful conversions and valid-base no-conversion cases do not clear
an existing errno value. `strtoul` accepts a leading minus and, when the parsed
magnitude is representable, returns the unsigned negation modulo the unsigned
long range; magnitude overflow still returns `ULONG_MAX` and sets `ERANGE`.
`malloc` uses `ENOMEM` for request-size overflow or when raw `brk` refuses heap
growth. `calloc` uses `ENOMEM` when `nmemb * size` would overflow. `realloc`
uses `ENOMEM` when the requested size cannot be represented or when a required
replacement allocation cannot be obtained. Successful allocation/resizing calls
do not clear an existing errno value. Raw
`mini_sys_*` calls remain separate from this libc error contract. `putchar` and
`puts` translate a negative raw `write` result to its positive libc `errno`
value. A zero-progress write while bytes remain is treated as `EIO` to prevent
an unbounded retry loop. Successful stdio writes do not clear an existing
`errno` value.

The current storage is suitable for the single-threaded runtime milestone only.
Future threading/TLS work must preserve the `errno` lvalue contract while making
the backing slot thread-local.

## Environment access ABI

`__mini_start` retains the original Linux process `envp` vector before entering
`main`. The implementation-only `__mini_set_envp` symbol stores that vector for
`getenv`; it is not declared by a public header. `getenv(name)` performs no heap
allocation and does not copy environment strings. A successful lookup returns a
pointer directly after the matching `NAME=` prefix in the original process
environment entry. That storage therefore has process-stack lifetime and is
shared with the `envp` visible to `main`.

Matching is exact: the requested name must be non-empty, contain no `=`, and be
followed immediately by `=` in an environment entry. Empty values are valid and
return a non-null pointer to the entry's terminating null byte. Missing names,
empty names, and names containing `=` return null. Lookups do not modify
`errno`. The current surface intentionally does not expose POSIX `environ`,
`setenv`, `putenv`, or `unsetenv`; adding mutation later must define how retained
storage and returned pointers are updated or invalidated.

## Write-only stdio ABI

`<stdio.h>` currently defines only `EOF`, `putchar`, and `puts`. There is no
`FILE` object model, `stdin`/`stdout`/`stderr` symbol, buffering, formatted I/O,
or input API yet. The implemented functions target the Linux process standard
output descriptor directly through raw `mini_sys_write(1, ...)`; this keeps the
observable standard-output behavior real without pretending a stream layer
exists.

`putchar(c)` converts `c` to `unsigned char`, writes exactly that byte, and
returns the byte converted back to `int` on success. `puts(s)` writes all bytes
before the terminating null and then one newline; it returns zero on success,
which satisfies the standard nonnegative-success contract. Positive short
writes advance the buffer and retry the unwritten suffix. A negative raw return
sets `errno` to the corresponding positive Linux errno value and returns `EOF`.
A zero raw return while bytes remain sets `EIO` and returns `EOF`. Partial output
may therefore be visible before a later failure, while the first failing raw
operation determines the reported errno.

These calls are deliberately unbuffered and single-operation-state-free.
Adding `FILE`, stream error indicators, buffering, or formatted I/O requires a
separate ABI design rather than silently changing this minimal surface.

## Allocator ABI and ownership

`malloc`/`calloc`/`realloc`/`free` currently form a single-threaded
`brk`-backed allocator for the x86-64 target. The allocator aligns its initial
heap frontier upward to 16 bytes, uses 32-byte in-band block headers, and returns
payload addresses aligned to 16 bytes. Each allocated header records both the
aligned payload capacity and the exact requested byte count; `realloc` uses the
latter so a move copies only bytes belonging to the old allocation. A
compile-time assertion locks the header size required by that target layout.

`malloc(0)` returns null without changing `errno`. `calloc` also returns null
without changing `errno` when either `nmemb` or `size` is zero. Before allocating,
`calloc` checks multiplication against `SIZE_MAX`; overflow returns null and sets
`ENOMEM`. Successful `calloc` allocations are zero-filled byte-for-byte.
`realloc(NULL, n)` follows `malloc(n)`. For a non-null pointer, size zero frees
the block and returns null without changing `errno`. Nonzero shrinking updates
the requested size and splits a useful remainder in place. Growth reuses an
adjacent free block when sufficient, can extend a tail allocation through raw
`brk`, and otherwise allocates a replacement, copies
`min(old_requested_size, new_size)` bytes, then frees the old block. If resize
fails, the old allocation remains owned by the caller with its contents intact.
Nonzero `malloc` requests are rounded up to 16-byte multiples with checked
arithmetic. A request whose rounding, header addition, or program-break calculation would
overflow returns null and sets `ENOMEM`. Heap growth succeeds only when raw `mini_sys_brk(target)` returns
the requested target; an unchanged break is treated as allocation failure and
also produces `ENOMEM`.

Freed blocks remain inside the allocator arena. Allocation uses first-fit reuse,
splits a free block only when the remainder can hold another header plus at
least one aligned payload unit, and `free` coalesces adjacent free blocks in
address order. The allocator does not currently contract the program break or
return tail pages to the kernel. `free(NULL)` is a no-op. Invalid pointers and
double-free are outside the supported C contract and are not diagnosed.

Once allocator state is initialized it owns the process program break. Calling
`mini_sys_brk` to move the break behind the allocator would invalidate its
metadata and is unsupported. Querying the break with `mini_sys_brk(NULL)` does
not mutate it and is safe. Thread safety, TLS-aware locking, tail-page return,
and an mmap-backed large-allocation path are later milestones.

## ELF expectations

Milestone executables are linked with the system static linker only as a
bootstrap tool. They contain no `PT_INTERP`, no `DT_NEEDED` dependency, and no
undefined symbols. `tests/verify-no-host-libc.sh` enforces these properties.
There is no dynamic loader or shared-library support yet.
