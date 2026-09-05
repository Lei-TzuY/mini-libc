# Signal runtime ABI and phase status

mini-libc now exposes a first executable C signal/runtime-control baseline on
Linux x86-64. The public surface is intentionally small:

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

The raw syscall layer adds:

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

## Executable evidence

The freestanding `signal_probe` installs a real `SIGTERM` handler through the
kernel, raises the signal, verifies that the handler runs before `raise`
returns, switches the disposition to `SIG_IGN`, verifies ignored delivery,
reinstalls the handler, and finally restores `SIG_DFL`. Successful operations
also verify `errno` preservation. The probe is subject to host-libc-independence
inspection.

A hosted deterministic harness replaces only `rt_sigaction`, `getpid`, `gettid`,
and `tgkill`. It locks the exact kernel-action field ordering, `SA_RESTORER`, the
restorer pointer, empty mask, signal-set size, previous-disposition return value,
negative-error mapping, calling-thread IDs, and success-path `errno`
preservation.

Pinned tiny-c compiles every production C file plus the shared syscall assembly.
Its existing runtime integration executable installs and executes a real signal
handler, verifies ignored delivery, and restores the default disposition. The
same executable is linked and run once with GNU `ld` and once with the pinned
mini-elf-toolchain, so handler entry and `rt_sigreturn` are exercised through
both static linkers.

## Phase boundary and next frontier

This is a C signal/runtime-control baseline, not a POSIX signal subsystem.
Public `sigaction`, signal sets, `sigprocmask`, pending/wait APIs, alternate
signal stacks, realtime signals, timers, and async-signal-safe library claims
remain outside the contract. No TLS-backed or thread-safe libc state is claimed.

The signal baseline now unlocks a higher-level C runtime lifecycle that was not
previously implementable correctly: abnormal and quick termination. A strong
next coherent frontier is `abort()` plus the C11 `at_quick_exit`/`quick_exit`
lifecycle, with explicit separation from normal `atexit` handlers and stdio
flushing. In particular, `abort()` must reach the default `SIGABRT` termination
path even when an installed handler returns, while quick termination must run
only its own callback registry and must not perform normal stdio cleanup.
