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
same termination sequence. After the callback registry is drained, `exit`
flushes every live writable stream through the private stdio registry and then
delegates to `_Exit(status)`. A stream write performed by an `atexit` callback is
therefore eligible for that final flush.

`_Exit(status)` bypasses both the callback registry and stdio flushing and calls
raw `mini_sys_exit` directly. mini-libc now has inherited and dynamically owned
`FILE` streams with fixed-size buffered output, block transfer, and positioning.
`fclose` unregisters streams before owned storage is released, so normal exit
only visits still-live streams. There is no `tmpfile` object or pathname cleanup
phase yet. The kernel still closes descriptors remaining open when the process
terminates.

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

The six-argument `mmap` wrapper and four-argument `openat` wrapper explicitly
move the fourth C argument from `rcx` to `r10` before executing `syscall`.

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
| `mini_sys_openat` | 257 |

The wrappers expose the kernel's raw `rax` convention rather than translating it
to POSIX libc results. For the ordinary integer-returning calls, failures remain
negative errno values in `[-4095, -1]`. Raw Linux `brk` is an important
exception: `mini_sys_brk(NULL)` returns the current program break, a successful
request returns the resulting break, and a refused request returns the unchanged
break rather than `-ENOMEM`. No raw wrapper updates libc `errno`.

## errno storage ABI

`<errno.h>` currently exposes the Linux values `ENOENT = 2`, `EIO = 5`,
`ENOMEM = 12`, `EINVAL = 22`, and `ERANGE = 34` and defines `errno` as a
modifiable `int` lvalue backed by `__mini_errno_location()`. The accessor
currently returns one process-global, zero-initialized BSS slot. This is
deliberately not thread-local because mini-libc has no TLS runtime yet. Keeping
access behind the implementation-reserved accessor allows a later TLS
implementation without changing source code that uses the `errno` macro.

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

The stdio stream layer maps negative raw I/O results to positive libc `errno`
values and sets the affected stream's error indicator. A zero-progress write
while bytes remain is treated as `EIO` so a flush cannot spin forever. Calling an
input operation on a write-only stream or an output operation on a read-only
stream is rejected with `EINVAL` and sets the stream error indicator. A true
end-of-file read is not an error: it sets only the EOF indicator and leaves an
existing `errno` unchanged. Successful stream operations and successful flushes
preserve an existing `errno` value.

`fread` and `fwrite` reject an unrepresentable `size * nmemb` request with
`EINVAL`, set the stream error indicator, and perform no transfer. This is a
deterministic defensive mini-libc extension rather than a portability claim
about arbitrary oversized caller objects. `fread` retries positive short raw
reads; buffered output retries positive short raw writes when a flush occurs.
Input transfer failures return only the number of complete elements; bytes from
a partially transferred final input element remain observable but do not
increment the return count.

A flush failure retains the unwritten suffix in the stream's private buffer. If
part of the buffer was written before the failure, those bytes are removed from
the pending prefix so a later retry does not duplicate them. `fflush(NULL)`
attempts all live writable streams and reports the first observed flush error
after continuing the registry walk.

`fseek` maps invalid stream/origin arguments to `EINVAL`; a raw seek failure maps
its negative kernel result to positive `errno`. Pending output must flush
successfully before a seek is issued. `ftell` reports raw seek errors the same way
and returns `-1L` on failure. `rewind` returns `void`, uses the synchronized seek
path, clears the stream EOF/error indicators, and leaves an underlying flush or
seek failure observable through `errno`.

`fopen` rejects a null filename or unsupported mode with `EINVAL` before issuing
`openat`. A pathname failure maps the negative raw `openat` result to positive
libc `errno`; for example, a missing read target reports `ENOENT`. If allocating
the private `FILE` object fails, allocator `ENOMEM` is preserved and no open
syscall is issued. `fclose` attempts pending-output flush before raw close, but it
still closes, unregisters, and releases owned stream state if flushing fails. If
both flush and close fail, the earlier flush error is retained as the reported
`errno`.

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

## Stream ABI

`<stdio.h>` exposes an opaque `FILE` type plus the predefined `stdin`, `stdout`,
and `stderr` expressions. Their public names resolve to implementation-reserved
`FILE *` objects, keeping the concrete stream layout out of the source-level ABI.
The three inherited objects are process-global and bind directly to Linux
descriptors 0, 1, and 2. `stdin` is readable; `stdout` and `stderr` are writable.
The private representation includes mode/state flags, a live-stream link, and a
fixed 256-byte output buffer. `stdout` and pathname-backed writable streams use
that buffer. `stderr` remains unbuffered. Input remains unbuffered in this
milestone.

`fgetc`, `getc`, and `getchar` read one byte through raw `mini_sys_read`. A
successful byte is returned as `unsigned char` converted to `int`. A raw zero
result sets the stream EOF indicator and returns `EOF`; once set, that indicator
is sticky and later reads return `EOF` without issuing another syscall until
`clearerr` or a successful positioning operation clears it. A negative raw
result sets the error indicator, updates `errno`, and returns `EOF`.

`fputc`, `putc`, `putchar`, `fputs`, `puts`, and `fwrite` share the same output
path. Buffered streams first copy output into their private buffer and flush when
additional data requires space or an explicit lifecycle boundary requests it.
Unbuffered `stderr` writes through the raw syscall loop immediately. Raw flushes
retry positive short writes. A negative raw return sets the stream error
indicator and `errno`; a zero-progress write sets `EIO`. A partial failing flush
retains only the bytes that have not yet reached the kernel.

`fread(ptr, size, nmemb, stream)` performs unbuffered block input. If either
`size` or `nmemb` is zero, it returns zero without issuing a syscall. For a
nonzero request, checked multiplication forms the requested byte count. Positive
short reads are retried until the request completes or a terminal condition
occurs. The function returns the number of complete `size`-byte elements read.
If EOF or an error arrives after some bytes of the next element were transferred,
those bytes remain in the caller's object but that incomplete element is not
counted.

`fwrite` checks the same size product but hands the resulting bytes to the shared
output core. Its return count is based on complete elements accepted before any
buffer-full flush failure prevents further acceptance. A zero raw write while a
flush still has pending bytes is converted to `EIO`.

`fflush(stream)` publishes pending output for one writable stream. A successful
flush clears the update-stream write-to-read synchronization barrier and leaves
`errno` unchanged. `fflush(NULL)` traverses the private process-global live-stream
registry and attempts every writable stream; it continues after failures and
returns `EOF` with the first error if any flush fails. Calling `fflush` on a live
read-only stream is a deterministic no-op extension in the current milestone.

Update streams track direction transitions explicitly. A write must be followed
by `fflush` or a successful positioning operation before input is accepted. A
non-EOF read must be followed by successful positioning before output is
accepted. Encountering EOF clears the read-to-write positioning requirement, so
output may follow that EOF directly. `clearerr` clears only the public EOF/error
indicators; it does not bypass a still-active direction barrier.

`feof` and `ferror` expose the sticky EOF/error indicators. `clearerr` clears both
without changing descriptor binding, stream mode, pending output, or update-
stream synchronization state.

`fopen` creates pathname-backed owned streams while reusing the same private
`FILE` representation. Mode strings currently support `r`, `w`, and `a`, with
at most one optional `+` and at most one optional `b` in either order. On the
Linux target, `b` does not change kernel flags. Plain `r` maps to read-only;
plain `w` maps to write-only plus `O_CREAT|O_TRUNC`; plain `a` maps to write-only
plus `O_CREAT|O_APPEND`. `+` selects read/write while retaining the create,
truncate, or append semantics implied by the first character. New files are
requested with mode `0666`, subject to the process umask. The current milestone
does not implement C11 exclusive-create `x` modes.

Before opening, `fopen` allocates private stream state through mini-libc `malloc`.
The pathname is then opened with `mini_sys_openat(AT_FDCWD, ...)`. If the open
fails, the temporary object is freed before the raw error is published through
`errno`. A successful owned stream stores its descriptor/capability flags and is
inserted into the live-stream registry only after both allocation and open
succeed. `fclose` attempts buffered output flush, then raw close, removes the
stream from the registry, and frees owned state. The predefined inherited streams
are not heap-owned; if explicitly closed they are invalidated in place and also
removed from the registry.

`SEEK_SET`, `SEEK_CUR`, and `SEEK_END` are the public origin values 0, 1, and 2
and map directly to raw Linux `lseek`. `fseek` accepts any live readable or
writable stream, first flushes pending output if the stream is writable, and only
then delegates the requested offset/origin to `mini_sys_lseek`. If the flush
fails, no seek occurs. On successful seek it clears EOF and both update-stream
direction barriers while preserving an existing stream error indicator. Because
this runtime is fixed to x86-64 Linux LP64, the public `long` offset used by
`fseek`/`ftell` matches the raw 64-bit seek boundary for this target; this is not
a cross-platform large-file portability claim.

`ftell` queries the current kernel offset through
`mini_sys_lseek(fd, 0, SEEK_CUR)` and adds pending buffered output bytes so a
caller observes the logical stream position before an ordinary flush occurs.
Append mode is a special case: with pending output, `ftell` first flushes and then
queries the kernel offset because `O_APPEND` determines the actual write location
at write time. A raw seek failure returns `-1L` and updates `errno` without
setting the stream error indicator. `rewind` reuses `fseek(stream, 0, SEEK_SET)`
for synchronization, then clears EOF and error indicators; because it returns
`void`, a flush or seek failure remains observable through `errno`.

The stream layer is single-threaded and the output buffer size is intentionally
fixed for this milestone. There is no input buffering, `setvbuf`, formatted I/O,
`fgetpos`/`fsetpos`, `tmpfile`, locking, or C11 exclusive-create `x` mode yet.
Future stdio work must extend this FILE state machine and live-stream lifecycle
rather than creating descriptor-specific side paths.

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
arithmetic. A request whose rounding, header addition, or program-break
calculation would overflow returns null and sets `ENOMEM`. Heap growth succeeds
only when raw `mini_sys_brk(target)` returns the requested target; an unchanged
break is treated as allocation failure and also produces `ENOMEM`.

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
linked and executed without the host libc. The pinned integration executable
creates data through buffered `fwrite`, uses `fseek`/`ftell` to overwrite a
positioned range, rewinds and reads it through `fread`, then seeks again after
EOF. Its final status line is written through buffered `stdout`, so successful
capture proves that normal `exit` flushed the live stream in both linker paths.
The harness independently verifies the final file bytes `012345XY89`. There is
no dynamic loader or shared-library support yet.
