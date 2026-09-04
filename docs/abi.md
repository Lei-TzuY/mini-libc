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
invokes `main(int, char **, char **)`, then passes the returned status to the
libc `exit` path. There are currently no constructors, destructors, TLS setup,
or other pre-main initialization hooks.

## Normal termination ABI

`<stdlib.h>` exposes `atexit`, `exit`, and `_Exit`. `atexit` records callbacks in
one process-global fixed-capacity registry. The registry accepts 32 simultaneous
registrations, satisfying the C minimum guarantee, and the same callback may be
registered more than once. Successful registration returns zero; a full registry
returns a nonzero value without disturbing existing entries. A null callback is
also rejected with a nonzero result as a defensive extension. The registry is
single-threaded state and is not TLS-backed or synchronized.

Normal termination happens either when `main` returns or when code calls
`exit(status)`. Registered callbacks are removed from the registry immediately
before they are invoked, so callbacks run in reverse registration order and a
callback registered by a callback during termination remains eligible for the
same termination sequence. After the registry is drained, `exit` delegates to
`_Exit(status)`.

`_Exit(status)` bypasses the callback registry and calls raw `mini_sys_exit`
directly. It therefore provides the immediate-termination path required when no
normal-exit callbacks should run. mini-libc now has inherited unbuffered standard
`FILE` streams, but no buffered stream state, dynamically owned streams, or
`tmpfile` objects, so `exit` has no user-space flush/close or temporary-file
cleanup phase yet. The kernel closes inherited descriptors when the process
terminates. Future buffering or owned-stream support must extend normal
termination rather than bypassing this callback boundary.

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
do not clear an existing errno value. Raw `mini_sys_*` calls remain separate
from this libc error contract.

The stdio stream layer maps a negative raw `read` or `write` result to the
corresponding positive libc `errno` value and sets that stream's error indicator.
A zero-progress write while bytes remain is treated as `EIO` to prevent an
unbounded retry loop. Calling an input operation on a write-only inherited stream
or an output operation on the read-only inherited stream is rejected with
`EINVAL` and sets the stream error indicator. A true end-of-file read is not an
error: it sets only the EOF indicator and leaves an existing `errno` unchanged.
Successful stream operations also leave an existing `errno` value unchanged.

The current errno storage is suitable for the single-threaded runtime milestone
only. Future threading/TLS work must preserve the `errno` lvalue contract while
making the backing slot thread-local.

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

## Standard stream ABI

`<stdio.h>` exposes an opaque `FILE` type plus the predefined `stdin`, `stdout`,
and `stderr` expressions. Their public names resolve to implementation-reserved
`FILE *` objects, keeping the concrete stream layout out of the source-level ABI.
The current three objects are process-global and bind directly to inherited Linux
descriptors 0, 1, and 2. `stdin` is readable; `stdout` and `stderr` are writable.
There is no dynamic stream creation or descriptor ownership transfer yet.

`fgetc`, `getc`, and `getchar` read one byte through raw `mini_sys_read`. A
successful byte is returned as `unsigned char` converted to `int`. A raw zero
result sets the stream EOF indicator and returns `EOF`; once set, that indicator
is sticky and later reads return `EOF` without issuing another syscall until
`clearerr` clears it. A negative raw result sets the error indicator, updates
`errno`, and returns `EOF`.

`fputc` and `putc` write the `unsigned char` conversion of their argument to the
selected stream. `putchar` is the `stdout` specialization. `fputs` writes all
bytes before the source terminator without appending anything; `puts` writes the
string to `stdout` and then appends one newline. Output retries positive short
writes until the requested bytes are consumed. A negative raw return sets the
stream error indicator and `errno`; a zero-progress write sets `EIO` so output
cannot spin forever. Successful output returns a nonnegative result and does not
clear an already-set error indicator.

`feof` and `ferror` expose the sticky EOF/error indicators. `clearerr` clears both
without changing descriptor binding or stream mode. The stream layer is
intentionally unbuffered and single-threaded. There is no `fflush`, `fread`,
`fwrite`, `fopen`, `fclose`, seeking API, stream allocator, locking, or formatted
I/O yet. Those capabilities must build on this object/state boundary rather than
reintroducing descriptor-specific stdio paths.

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

Milestone executables contain no `PT_INTERP`, no `DT_NEEDED` dependency, and no
undefined symbols; `tests/verify-no-host-libc.sh` enforces these properties. The
ordinary local `make` path still uses the system assembler, archive tool, and
static linker as bootstrap tools. CI additionally pins `tiny-c-compiler` and
proves that it can compile every production C source, then pins
`mini-elf-toolchain` and proves that the resulting mini-libc objects can be
linked and executed without the host libc. There is no dynamic loader or
shared-library support yet.
