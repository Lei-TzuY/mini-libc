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

Except for the non-returning exit call, wrappers return the kernel's raw `rax`
value. Errors therefore remain negative errno values in `[-4095, -1]`; raw
syscall wrappers still do not translate failures to `-1` or update libc
`errno`.

## errno storage ABI

`<errno.h>` currently exposes `ERANGE` with the Linux value 34 and defines
`errno` as a modifiable `int` lvalue backed by `__mini_errno_location()`.
The accessor currently returns one process-global, zero-initialized BSS slot.
This is deliberately not thread-local because mini-libc has no TLS runtime yet.
Keeping access behind the implementation-reserved accessor allows a later TLS
implementation without changing source code that uses the `errno` macro.

The current storage is suitable for the single-threaded runtime milestone only.
Future threading/TLS work must preserve the `errno` lvalue contract while making
the backing slot thread-local.

## ELF expectations

Milestone executables are linked with the system static linker only as a
bootstrap tool. They contain no `PT_INTERP`, no `DT_NEEDED` dependency, and no
undefined symbols. `tests/verify-no-host-libc.sh` enforces these properties.
There is no dynamic loader or shared-library support yet.
