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

`__mini_start` decodes `argc`, `argv`, and `envp`, invokes
`main(int, char **, char **)`, then forwards the returned status to
`mini_sys_exit`. There are currently no constructors, destructors, TLS setup,
`atexit` handlers, or libc initialization hooks.

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

`<errno.h>` currently exposes the Linux values `ENOMEM = 12`, `EINVAL = 22`, and
`ERANGE = 34` and defines `errno` as a modifiable `int` lvalue backed by
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
growth; successful allocations do not clear an existing errno value. Raw
`mini_sys_*` calls remain separate from this libc error contract.

The current storage is suitable for the single-threaded runtime milestone only.
Future threading/TLS work must preserve the `errno` lvalue contract while making
the backing slot thread-local.

## Allocator ABI and ownership

`malloc`/`free` currently form a single-threaded `brk`-backed allocator for the
x86-64 target. The allocator aligns its initial heap frontier upward to 16 bytes,
uses 32-byte in-band block headers, and returns payload addresses aligned to 16
bytes. A compile-time assertion locks the header size required by that target
layout.

`malloc(0)` returns null without changing `errno`. Nonzero requests are rounded
up to 16-byte multiples with checked arithmetic. A request whose rounding,
header addition, or program-break calculation would overflow returns null and
sets `ENOMEM`. Heap growth succeeds only when raw `mini_sys_brk(target)` returns
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
not mutate it and is safe. Thread safety, TLS-aware locking, `calloc`, `realloc`,
and an mmap-backed large-allocation path are later milestones.

## ELF expectations

Milestone executables are linked with the system static linker only as a
bootstrap tool. They contain no `PT_INTERP`, no `DT_NEEDED` dependency, and no
undefined symbols. `tests/verify-no-host-libc.sh` enforces these properties.
There is no dynamic loader or shared-library support yet.
