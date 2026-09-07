# Termination runtime lifecycle

mini-libc exposes four deliberately distinct C/C11 termination paths on Linux
x86-64:

```text
normal termination
  -> exit(status)
  -> synchronized normal atexit registry, LIFO
  -> flush every live writable FILE
  -> _Exit(status)
  -> SYS_exit_group

last C11 user-thread termination
  -> thrd_exit(result)
  -> TSS destructors for the terminating thread
  -> if another C11 user thread remains: SYS_exit for this thread only
  -> if this is the last C11 user thread: exit(EXIT_SUCCESS)

quick termination
  -> quick_exit(status)
  -> synchronized quick-exit registry, LIFO
  -> _Exit(status)
  -> SYS_exit_group

abnormal termination
  -> abort()
  -> SIGABRT delivery
  -> if a handler returns: install SIG_DFL
  -> SIGABRT delivery again
  -> emergency _Exit(134) only if signal delivery still returns
```

These paths intentionally do not share callback registries or cleanup policy.
The separation is part of the executable runtime contract, not merely an
implementation detail.

## Public surface

`<stdlib.h>` exposes:

```c
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

int atexit(void (*func)(void));
int at_quick_exit(void (*func)(void));

_Noreturn void exit(int status);
_Noreturn void quick_exit(int status);
_Noreturn void _Exit(int status);
_Noreturn void abort(void);
```

`<threads.h>` independently exposes `thrd_exit`. The thread runtime now connects
that API back to normal process termination when the final C11 user thread
terminates.

Both callback registries retain a fixed capacity of 32 entries and remain
process-global. Null callback registration is rejected deterministically with
`-1`. Capacity exhaustion also returns `-1` without disturbing previously
registered callbacks.

Unlike the earlier single-thread baseline, registration and registry removal are
now serialized with the same private C11 `atomic_int` + futex lock abstraction
used by the other synchronized runtime serializers. Concurrent registration
therefore does not race the registry count or callback slots.

The registry lock is held only while checking, appending, or popping a slot. It
is always released before calling user code. A callback may consequently call
`atexit` or `at_quick_exit` again without deadlocking; a newly registered
callback becomes the next applicable LIFO entry if capacity is available after
the current callback was popped.

This synchronization does not invent semantics for multiple threads racing
process-termination calls. The C/C11 contracts do not require mini-libc to make
concurrent `exit`/`quick_exit` invocations into a new multi-winner lifecycle.
The proven claim is data-race-free callback registration plus correct single
termination-owner draining.

## Normal termination

`atexit()` records callbacks in the synchronized normal registry. `exit()` pops
and calls them in reverse registration order. Each pop is atomic with respect to
registration, but arbitrary user code runs outside the registry lock. After the
registry is drained, `exit()` invokes the synchronized stdio live-stream flush
sweep and finally delegates to `_Exit()`.

Returning from `main` follows this same path through the crt startup layer. A
normal return from `main` or an explicit `exit()` does not run callbacks
registered by `at_quick_exit()`.

## Last C11 user thread

The thread runtime maintains one C11 atomic live-user count. The initial main
thread contributes one. A successful `thrd_create` reserves the new user's count
**before** issuing raw `clone`, so a child that runs and exits immediately cannot
be misclassified as the last user while its parent is still returning from
`clone`. Clone failure rolls that reservation back.

The internal detached-thread reaper is deliberately excluded from this count. It
is a process-lifetime implementation service created directly through the raw
clone entry, not through public `thrd_create`.

`thrd_exit` first runs the current thread's TSS destructor iterations and stores
the result in its public thread control when one exists. It then atomically drops
one live-user reference:

- if another C11 user thread remains, the current thread terminates through raw
  `SYS_exit` exactly as before;
- if the previous count was one, this was the last C11 user thread and the call
  transfers to `exit(EXIT_SUCCESS)`.

The second case is intentionally normal process termination: registered normal
callbacks run, buffered stdio is flushed, and `_Exit` finally uses
`SYS_exit_group`. A still-running internal reaper cannot keep the process alive
or suppress that cleanup.

Running TSS destructors before dropping the user count also preserves a crucial
creation edge: a destructor that successfully creates another C11 thread raises
the count before the terminating thread decides whether it is last.

## Quick termination

`at_quick_exit()` records callbacks in a separate synchronized registry.
`quick_exit()` drains only this registry in reverse order and then calls
`_Exit()`. It does not run normal `atexit()` callbacks and does not invoke the
stdio flush sweep.

Pending buffered stdio therefore remains unpublished unless the program flushed
it explicitly before calling `quick_exit()`. Quick-exit callbacks may register
additional quick-exit callbacks while the drain is in progress because the
registry lock is not held across the callback invocation.

## Immediate termination

`_Exit()` remains the minimal direct program-termination primitive. It performs
no callback execution and no stdio cleanup. In the thread-aware runtime it issues
raw Linux `SYS_exit_group`, not `SYS_exit`, so every thread in the process is
terminated. It is also the final primitive used by both normal and quick
termination once their respective lifecycle work is complete.

The raw syscall distinction remains explicit: `mini_sys_exit` maps to
`SYS_exit` and backs non-final C11 thread termination, while
`mini_sys_exit_group` backs C process termination. Once real `CLONE_THREAD`
threads exist those calls are observably different and are not interchangeable.

## Abnormal termination

`abort()` is implemented in the signal object so programs that only use normal
or quick termination do not pull the signal subsystem into the static link.
`abort()` raises `SIGABRT` once, permitting a previously installed handler to
run. If control returns, it resets the disposition to `SIG_DFL` and raises
`SIGABRT` again. This makes a returning handler unable to turn `abort()` into a
normal return.

Normal and quick callback registries are not drained, and stdio is not flushed.
An emergency `_Exit(134)` is present only if the signal path unexpectedly returns
again. The public signal baseline still makes no broader async-signal-safety
claim for arbitrary libc functions.

## Executable evidence

The existing single-thread runtime probes remain in place and continue to prove:

- normal return and explicit `exit()` retain reverse-order `atexit` behavior;
- `_Exit()` bypasses callbacks and buffering;
- `quick_exit()` runs quick handlers while ignoring normal handlers and pending
  buffered stdout;
- normal and quick registry capacity/null-registration failure remain
  deterministic;
- `abort()` executes the real `SIGABRT` path without draining callbacks or stdio.

The thread-aware termination probe adds three bounded process modes:

1. **normal registry contention**: ten C11 workers concurrently register three
   normal callbacks each. Together with an oldest final checker and newest
   reentrant callback the 32-slot registry is full, and one extra registration
   must fail. The reentrant callback registers a fresh callback after its own
   slot has been popped. The final checker requires all 30 worker callbacks, the
   reentrant callback, and the late callback to have run exactly as expected.
2. **quick registry contention**: the same concurrent/reentrant capacity contract
   is executed against `at_quick_exit`/`quick_exit`.
3. **last-user-thread cleanup**: main buffers `B`, registers an `atexit` handler
   that buffers `H`, creates and detaches one blocked user worker so the internal
   reaper is definitely running, releases that worker, and calls `thrd_exit`.
   Whichever of main or the detached worker becomes the last C11 user thread must
   perform normal exit. The process must terminate successfully and emit exactly
   `BH`; the historical raw-`SYS_exit` behavior would leave only the internal
   reaper alive and hang until the test timeout.

All three modes run as freestanding static ELFs and are covered by the
host-libc-independence inspection set.

Pinned tiny-c independently compiles termination modes that fill the normal
callback registry from concurrent C11 workers, exercise reentrant registration,
and run the last-user-thread/reaper/stdio-flush path. The same executable is
linked and executed through GNU `ld` and the pinned mini-elf-toolchain. Existing
tiny-c quick-exit and abort modes remain unchanged, so cross-toolchain evidence
covers the complete normal/last-thread/quick/abnormal matrix rather than only
new symbols.

The older active-child `_Exit(37)` regression also remains: one sibling waits and
would write `survived` if only the calling thread died. The test still requires
status 37 and empty output, preserving proof that direct process termination uses
`SYS_exit_group`.

## Phase boundary and promotion

This phase closes the thread-aware termination convergence gap for the current
static x86-64 runtime:

- normal and quick callback registries are safe for concurrent registration;
- callback draining remains LIFO and supports reentrant registration without
  holding a runtime lock across user code;
- C11 user-thread lifetime is counted independently of the internal reaper;
- non-final `thrd_exit` remains thread-local;
- final-user `thrd_exit` performs `exit(EXIT_SUCCESS)` cleanup;
- normal stdio flushing, quick no-flush behavior, direct `_Exit`, and signal-backed
  abnormal termination remain distinct.

There is no value in farming another callback-registry locking variant. The next
repository phase should be selected from the remaining live standard-runtime or
cross-subsystem capability gaps after a fresh architecture audit, rather than
adding more termination modes or weakening the existing process/thread boundary.
