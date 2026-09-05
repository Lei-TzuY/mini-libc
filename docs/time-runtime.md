# Time runtime ABI and phase status

mini-libc now exposes a first C11 time runtime backed by the raw Linux x86-64
`clock_gettime` syscall rather than host libc state. The public surface is:

```c
typedef long clock_t;
typedef long time_t;

struct timespec {
    time_t tv_sec;
    long tv_nsec;
};

#define CLOCKS_PER_SEC 1000000L
#define TIME_UTC 1

clock_t clock(void);
double difftime(time_t time1, time_t time0);
time_t time(time_t *timer);
int timespec_get(struct timespec *ts, int base);
```

## Kernel boundary

The raw boundary is:

```c
long mini_sys_clock_gettime(int clockid, void *tp);
```

On Linux x86-64 it issues syscall 228 and returns the kernel result directly.
It does not set `errno`. The public time layer maps negative kernel results to
positive libc `errno` values.

The target contract is explicitly LP64 x86-64. Compile-time assertions require
an 8-byte `long`, 8-byte `time_t`, and 16-byte `struct timespec`, matching the
kernel layout used by the raw syscall.

## Wall-clock time

`time()` and `timespec_get(..., TIME_UTC)` sample Linux `CLOCK_REALTIME`.
Successful calls preserve the incoming `errno` value.

`time(timer)` returns whole seconds since the Unix epoch and, when `timer` is
non-null, stores the same value through that pointer. A kernel failure returns
`(time_t)-1` and exposes the positive kernel error through `errno`; an output
pointer is not modified on failure.

`timespec_get(ts, TIME_UTC)` returns `TIME_UTC` after a successful realtime
sample. mini-libc deterministically rejects a null destination or unsupported
base with `0` and `EINVAL`. A raw clock failure also returns `0` with the mapped
kernel error.

## Process CPU time

`clock()` samples Linux `CLOCK_PROCESS_CPUTIME_ID`. `CLOCKS_PER_SEC` is one
million, so nanoseconds are converted to microsecond ticks by integer division.
The implementation validates the kernel nanosecond field and checks both the
seconds multiplication and final tick addition before constructing a `clock_t`.
Unavailable clocks or conversion overflow return `(clock_t)-1`; malformed or
unrepresentable samples use `ERANGE`.

This is process CPU time, not elapsed wall time. The origin of the clock value is
implementation-defined as allowed by C; callers should compare samples and divide
the difference by `CLOCKS_PER_SEC`.

## `difftime`

`difftime(time1, time0)` converts both `time_t` operands to `double` before
subtracting them. This avoids signed-integer subtraction overflow even when the
input values span the complete 64-bit `time_t` range. As with any binary64 result,
very large integer timestamps can lose unit precision after conversion.

## Executable evidence

The freestanding `time_probe` executes real kernel clocks. It checks a valid UTC
`timespec`, consistency between `time()` and a nearby realtime sample, monotonic
nondecrease of process CPU time around actual work, success-path `errno`
preservation, and deterministic rejection of an unsupported `timespec_get` base.
The executable is also included in the host-libc-independence inspection.

A hosted fake-clock harness replaces only the raw `clock_gettime` boundary. It
locks exact clock IDs, result-unit conversion, raw negative-error mapping,
no-write-on-`time()` failure, invalid-base/no-syscall behavior, CPU-time overflow,
invalid nanosecond handling, and extreme-range `difftime` behavior.

Pinned tiny-c builds every production C file, including the time runtime, into the
same mini-libc archive. A dedicated tiny-c time executable then calls
`timespec_get`, `time`, `difftime`, and `clock` against real kernel clocks. The
same executable is linked and run through GNU `ld` and the pinned
mini-elf-toolchain and remains subject to host-libc-independence inspection.

## Phase boundary and next frontier

This phase establishes a kernel-backed scalar time baseline; it does not claim a
complete calendar, timezone, locale, or sleep subsystem. `gmtime`, `localtime`,
`mktime`, `strftime`, timezone databases, and sleep/timer APIs remain outside the
current contract.

A thread-local/reentrant runtime phase was explicitly audited before this work.
The pinned tiny-c compiler currently exposes no `_Thread_local`/`__thread` codegen
surface, and the pinned mini-elf linker exposes no x86-64 TLS relocations or
`PT_TLS` handling. mini-libc therefore does not fake TLS-backed `errno` or other
thread-safety claims.

The previously identified signal/runtime-control frontier is now implemented by
the independent signal baseline documented in `docs/signal-runtime.md`. That
milestone also establishes the kernel mechanism needed for a higher-level C
runtime lifecycle frontier: `abort()` plus C11 `at_quick_exit`/`quick_exit`, with
explicit separation between abnormal termination, quick-exit callbacks, normal
`atexit` callbacks, and stdio flushing. Calendar conversion remains independent
future work and should still state its static-storage, timezone, and locale
contracts without pretending unavailable TLS support.
