# Signal runtime ABI and phase status

mini-libc exposes an executable C signal/runtime-control baseline on Linux
x86-64. The public signal surface remains intentionally small:

```c
typedef int sig_atomic_t;

#define SIG_DFL ((void (*)(int))0)
#define SIG_IGN ((void (*)(int))1)
#define SIG_ERR ((void (*)(int))-1)

#define SIGINT  2
#define SIGILL  4
#define SIGABRT 6
#define SIGFPE  8
#define SIGSEGV 11
#define SIGTERM 15

void (*signal(int sig, void (*func)(int)))(int);
int raise(int sig);
```

`abort()` is declared by `<stdlib.h>` but deliberately builds on this same
signal boundary, so abnormal termination and signal delivery share one kernel
contract rather than maintaining a private second path.

## Linux x86-64 kernel boundary

`signal()` is implemented through the raw `rt_sigaction` syscall rather than a
host-libc wrapper. The private kernel action layout is exactly four machine
words in this order: handler, flags, restorer, mask. A compile-time assertion
requires the structure to remain 32 bytes on the LP64 target.

The installed action uses Linux x86-64 `SA_RESTORER` (`0x04000000`) and an
8-byte kernel signal-set size. The restorer is a hidden assembly trampoline that
issues `rt_sigreturn` directly. A small assembly accessor returns the trampoline
address from the same syscall object, keeping the static link surface on the
relocation types already supported by both GNU `ld` and the pinned
mini-elf-toolchain.

The raw syscall layer provides:

```c
long mini_sys_rt_sigaction(int sig, const void *act, void *oldact,
                           unsigned long sigsetsize);
long mini_sys_getpid(void);
long mini_sys_gettid(void);
long mini_sys_tgkill(int tgid, int tid, int sig);
```

These wrappers expose kernel return conventions and do not set libc `errno`.
The public signal layer maps negative kernel results to positive `errno` values.

## `signal()` contract

A successful `signal(sig, handler)` installs the requested handler, `SIG_DFL`,
or `SIG_IGN`, returns the previous disposition, and preserves the incoming
`errno`. A raw `rt_sigaction` failure returns `SIG_ERR` and exposes the positive
kernel error through `errno`.

Handlers installed by this baseline remain installed after delivery, matching
the Linux `rt_sigaction` behavior used by the implementation. Returning from a
handler executes the private `rt_sigreturn` restorer and resumes the interrupted
context.

The handler mask is empty and this phase does not expose public signal-mask
manipulation. The implementation does not claim that arbitrary libc functions
are async-signal-safe.

## `raise()` contract

`raise(sig)` targets the calling Linux thread rather than merely the process. It
samples the current process and thread IDs through `getpid` and `gettid`, then
issues `tgkill(pid, tid, sig)`. On success it returns zero and preserves the
incoming `errno`; a `tgkill` failure returns `-1` and maps the kernel error to
libc `errno`.

This thread-directed implementation remains useful even though the rest of the
runtime is currently single-threaded and has no public thread API: it states the
correct delivery contract without pretending unavailable TLS support.

## `abort()` integration

`abort()` first raises `SIGABRT` through the public signal layer. This permits an
already-installed C handler to run once. If that handler returns, or if the first
delivery otherwise returns to libc, `abort()` installs `SIG_DFL` for `SIGABRT`
and raises the signal a second time. The second delivery therefore follows the
default abnormal-termination path instead of returning to the caller.

If both signal operations unexpectedly return, an emergency `_Exit(128 +
SIGABRT)` remains as a last-resort non-returning path. This fallback is not the
normal observable termination mechanism on Linux; the real executable
regression requires termination by the second `SIGABRT` delivery.

`abort()` does not run normal `atexit` handlers, C11 quick-exit handlers, or
stdio flushing. The current signal baseline does not expose signal masks, so it
does not claim the broader POSIX `abort()` guarantee for a caller that has
explicitly blocked `SIGABRT` through APIs outside mini-libc's public contract.

## Executable evidence

The freestanding `signal_probe` installs a real `SIGTERM` handler through the
kernel, raises the signal, verifies that the handler runs before `raise`
returns, switches the disposition to `SIG_IGN`, verifies ignored delivery,
reinstalls the handler, and finally restores `SIG_DFL`. Successful operations
also verify `errno` preservation. The probe is subject to host-libc-independence
inspection.

A hosted deterministic harness replaces `rt_sigaction`, `getpid`, `gettid`, and
`tgkill`. It locks the exact kernel-action field ordering, `SA_RESTORER`, the
restorer pointer, empty mask, signal-set size, previous-disposition return value,
negative-error mapping, calling-thread IDs, and success-path `errno`
preservation. The same harness now intercepts the emergency `_Exit` path and
locks `abort()` sequencing as `raise(SIGABRT) -> signal(SIGABRT, SIG_DFL) ->
raise(SIGABRT) -> emergency _Exit`, including the fallback status 134.

The real runtime termination probe installs a `SIGABRT` handler that emits one
raw marker and returns. `abort()` must then terminate the process with signal
status 134; registered normal and quick-exit callbacks and pending buffered
stdout must remain invisible. This verifies the observable abnormal-termination
boundary rather than only the fake-syscall sequence.

Pinned tiny-c compiles every production C file plus the shared syscall assembly.
A dedicated tiny termination executable runs both quick termination and a real
returning `SIGABRT` handler. The same executable is linked and run once with GNU
`ld` and once with the pinned mini-elf-toolchain, so signal handler entry,
`rt_sigreturn`, disposition reset, and the final abnormal delivery are exercised
through both static linkers.

## Phase boundary and next frontier

The C signal/abnormal-termination baseline is now executable, but it is not a
POSIX signal subsystem. Public `sigaction`, signal sets, `sigprocmask`,
pending/wait APIs, alternate signal stacks, realtime signals, timers, and
async-signal-safe library claims remain outside the contract. No TLS-backed or
thread-safe libc state is claimed.

With ordinary termination, quick termination, and signal-backed abnormal
termination now separated, the next higher-value C runtime-control gap is
non-local control flow. A coherent next frontier is `<setjmp.h>` with an
x86-64 SysV `jmp_buf`, `setjmp`, and `longjmp`: preserve the required callee-saved
registers, stack pointer, and resume program counter; normalize `longjmp(..., 0)`
to a `setjmp` return value of 1; prove nested and cross-function recovery; and
run the same executable through GCC, Clang, pinned tiny-c, and mini-elf. Signal
mask preservation (`sigsetjmp`/`siglongjmp`) remains a later POSIX extension and
must not be implied by that C baseline.
