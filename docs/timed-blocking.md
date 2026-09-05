# Timed blocking synchronization

This document records the executable timed-blocking extension to mini-libc's
C11 thread runtime. It supersedes the earlier timed-blocking roadmap paragraph
in `docs/thread-runtime.md`; the broader thread-lifecycle, mutex, condition,
allocator, TLS, and detached-reaper contracts remain documented there.

## Public surface

`<threads.h>` provides:

```c
int cnd_timedwait(cnd_t *restrict cond, mtx_t *restrict mtx,
                  const struct timespec *restrict time_point);
int thrd_sleep(const struct timespec *duration,
               struct timespec *remaining);
```

Including `<threads.h>` also makes the C11 `struct timespec` definition available
through `<time.h>`.

The subsequent typed-mutex phase has now added `mtx_recursive`, `mtx_timed`,
recursive+timed combinations, and `mtx_timedlock`; those semantics and their
runtime evidence are authoritative in `docs/mutex-types.md`. `thrd_yield`,
`once_flag` / `call_once`, and the TSS APIs remain outside the completed surface.

## Absolute condition deadline

`cnd_timedwait` uses an absolute `TIME_UTC` / Linux `CLOCK_REALTIME` deadline.
The raw kernel wait is:

```text
futex(cond_sequence,
      FUTEX_WAIT_BITSET | FUTEX_CLOCK_REALTIME,
      expected_generation,
      absolute_timespec,
      NULL,
      FUTEX_BITSET_MATCH_ANY)
```

The condition generation is sampled while the caller still owns the mutex. The
mutex is then released before entering the futex wait. If a signal increments
the generation between unlock and kernel sleep, the futex expected-value check
returns `EAGAIN`; mini-libc treats that as a completed wait rather than sleeping
on a wake that already happened.

`EINTR` retries the same absolute deadline, so interruption cannot extend the
caller-selected timeout. Kernel `ETIMEDOUT` maps to `thrd_timedout`. Other raw
futex failures map to `thrd_error`.

As with `cnd_wait`, the mutex is reacquired before every return after a successful
unlock. A reacquisition failure is `thrd_error`, even if the kernel wait itself
ended by timeout. The caller's pre-existing `errno` is restored before return.
Invalid pointers or a nanosecond field outside `[0, 1000000000)` are rejected as
`thrd_error` without releasing the mutex. A negative absolute seconds field is
already expired and returns `thrd_timedout` after the same unlock/relock
transition without issuing a futex syscall.

Condition waits may still wake spuriously. Correct code protects a predicate
with the associated mutex and loops around either `cnd_wait` or
`cnd_timedwait`.

## Thread sleep

`thrd_sleep` is backed by the raw Linux x86-64 `nanosleep` syscall (number 35).
The duration is relative and must have non-negative seconds plus a nanosecond
field in `[0, 1000000000)`.

The return contract is:

- `0` when the requested interval completes;
- `-1` when the kernel reports `EINTR`; if `remaining` is non-null, the kernel
  writes the unslept interval there;
- `-2` for invalid input or another raw syscall failure.

`duration` and `remaining` may designate the same object. The syscall consumes
the request before writing the remainder, and deterministic regression coverage
locks that aliasing contract. The caller's pre-existing `errno` is preserved on
all public return paths.

## Executable evidence

The hosted condition harness remains scheduler-independent. In addition to the
existing generation-race and untimed error paths it proves:

- the exact `FUTEX_WAIT_BITSET | FUTEX_CLOCK_REALTIME` operation and
  `FUTEX_BITSET_MATCH_ANY` value;
- forwarding of the caller's absolute `timespec` without relative-time
  conversion;
- signal-between-unlock-and-wait completion through `EAGAIN`;
- `ETIMEDOUT -> thrd_timedout` with mandatory mutex reacquisition;
- `EINTR` retry against the same deadline;
- unexpected futex errors and timeout-plus-relock-failure handling;
- already-expired and invalid deadline behavior;
- `thrd_sleep` success, interruption with remaining time, request/remaining
  aliasing, invalid durations, raw errors, and errno preservation.

The freestanding condition probe keeps the original six-worker signal/broadcast
scenario, then creates a real worker that sleeps briefly with `thrd_sleep`,
locks the shared mutex, changes the predicate, and signals the condition. The
main thread waits through `cnd_timedwait` with a future `TIME_UTC` deadline and
must wake successfully. A second wait uses a deadline in the past and must return
`thrd_timedout` while holding the mutex again.

The pinned tiny-c condition executable performs the same future-deadline wake and
expired-deadline timeout through code produced by the pinned compiler. That
executable is linked and run both with GNU `ld` and the pinned mini-elf-toolchain,
so the raw nanosleep boundary, absolute futex ABI, C11 public prototypes, and
thread scheduler behavior are all exercised through the cross-repository gate.

## Runtime-wide synchronization status

The thread runtime now includes dynamic join/detach ownership, per-thread errno,
plain/recursive/timed mutexes, untimed and timed condition waits, relative thread
sleep, condition broadcast/signal, a synchronized allocator, and the recursive
futex-backed shared FILE/registry synchronization layer. Memory-only formatted
I/O remains independent of the shared FILE lock.

This still is not a claim that every process-global libc subsystem is fully
thread-safe. Termination callback registries, environment mutation/storage, and
hidden string continuation state require their own ownership contracts.

## Phase boundary and promotion

Timed condition waiting and thread sleep closed the first timed-blocking phase.
The following mutex-type phase has now also closed recursive ownership and
absolute `mtx_timedlock` semantics; see `docs/mutex-types.md`.

The next larger C11 lifecycle frontier is **exactly-once initialization and
thread-specific storage**: `once_flag` / `call_once` plus `tss_t` creation,
get/set/delete, and destructor execution during thread exit. That work needs a
process-wide once state machine and per-thread destructor lifecycle rather than
additional condition or mutex variants.
