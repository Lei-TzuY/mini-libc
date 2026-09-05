# Typed and timed mutex semantics

This document records the executable C11 mutex-type phase layered on top of the
existing Linux x86-64 thread, futex, TIME_UTC, and timed-blocking runtime. It
supersedes the earlier plain-mutex-only status in `docs/thread-runtime.md` and
the mutex roadmap paragraph in `docs/timed-blocking.md`.

## Public surface

`<threads.h>` now exposes the mutex type bits:

```c
enum {
    mtx_plain = 0,
    mtx_recursive = 1,
    mtx_timed = 2
};
```

`mtx_recursive | mtx_timed` is a supported combined type. Any other type bits
are rejected by `mtx_init` with `thrd_error`.

The public API adds:

```c
int mtx_timedlock(mtx_t *restrict mtx,
                  const struct timespec *restrict time_point);
```

The existing `mtx_init`, `mtx_lock`, `mtx_trylock`, `mtx_unlock`, and
`mtx_destroy` calls now operate on typed mutex state.

## State and ownership

The mutex runtime is no longer embedded in the thread lifecycle translation
unit. `src/thread/lifecycle.c` owns create/join/detach/reaper behavior, while
`src/thread/mutex.c` owns the mutex state machine. This keeps typed/timed growth
out of thread-control lifetime code and leaves only one public `mtx_*`
implementation in the archive.

A mutex records:

- one 32-bit held/free state word used by the futex boundary;
- the initialized type mask;
- an owner identity token;
- the recursive acquisition depth.

The owner token is the current mini-libc TCB pointer obtained through the
existing `%fs:0` accessor. Acquiring an uncontended mutex therefore does not add
a `gettid` syscall to the fast path. Pointer-sized owner loads/exchanges use
private x86-64 atomic helpers; state/depth retain the existing 32-bit exchange
and fetch-add primitives.

Final unlock clears depth and owner before publishing state zero and waking one
futex waiter. A caller that does not own the mutex receives `thrd_error` and
does not mutate the held state. This gives wrong-thread unlock a deterministic
mini-libc contract instead of silently releasing another thread's mutex.

## Plain and recursive acquisition

`mtx_plain` keeps the allocation-free atomic/futex model. An uncontended
acquisition claims the state word and records the current owner/depth. A
contended caller waits with raw `FUTEX_WAIT`, retrying `EINTR` and `EAGAIN`.
Unexpected raw futex failures return `thrd_error`.

`mtx_trylock` returns `thrd_busy` when a non-recursive mutex cannot be acquired.
For the current owner of a recursive mutex, both `mtx_lock` and `mtx_trylock`
increment recursion depth without entering the kernel. Intermediate recursive
unlocks only decrement the depth; the final unlock releases the futex state and
wakes one waiter. Depth overflow is rejected with `thrd_error`.

For deterministic failure rather than self-deadlock, mini-libc returns
`thrd_error` when the current owner calls blocking `mtx_lock` again on a
non-recursive mutex. This is a mini-libc behavior choice for an otherwise invalid
usage pattern, not a broader portability claim.

## Timed acquisition

`mtx_timedlock` is accepted only for a mutex initialized with the `mtx_timed`
bit and a non-null absolute `struct timespec` whose nanoseconds are in
`[0, 1000000000)`.

An immediately available mutex is acquired even when the absolute deadline is
already in the past. Recursive self-acquisition likewise succeeds without a
kernel wait. Only a genuinely contended mutex consults the deadline.

The raw timed wait is:

```text
futex(state,
      FUTEX_WAIT_BITSET | FUTEX_CLOCK_REALTIME,
      1,
      absolute_timespec,
      NULL,
      FUTEX_BITSET_MATCH_ANY)
```

`EINTR` and `EAGAIN` retry against the same caller-provided absolute deadline;
interruption cannot extend the timeout. Kernel `ETIMEDOUT` maps to
`thrd_timedout`. An already-expired deadline on a contended mutex returns
`thrd_timedout` without issuing a futex syscall. Other raw failures map to
`thrd_error`.

All public mutex calls preserve the caller's pre-existing `errno` on success,
busy, timeout, and error returns. The `thrd_*` result is the public failure
channel.

## Condition-variable boundary

The condition-variable implementation from the preceding phase is intentionally
unchanged here. Its release/wait/reacquire protocol continues to call public
`mtx_unlock` and `mtx_lock`.

This phase does not claim special semantics for calling `cnd_wait` or
`cnd_timedwait` while a recursive mutex is held at depth greater than one.
Defining a full recursive-depth handoff/restoration contract would be a separate
condition/mutex integration problem and is not required to establish recursive
or timed mutex acquisition itself.

## Executable evidence

The hosted deterministic mutex harness controls the current TCB identity and raw
futex results without scheduler timing. It proves:

- valid plain, recursive, timed, and recursive+timed initialization;
- rejection of invalid type masks and invalid deadlines;
- plain try-lock busy behavior and deterministic same-owner blocking error;
- wrong-owner unlock rejection without state mutation;
- recursive lock/try-lock depth growth and wake only on final unlock;
- recursion-depth overflow rejection;
- timed-lock rejection on a non-timed mutex;
- immediate acquisition despite an expired deadline when the mutex is free;
- no-syscall timeout for an already-expired contended deadline;
- the exact realtime absolute futex operation, deadline, expected value, and
  bitset;
- `ETIMEDOUT -> thrd_timedout`;
- `EINTR` plus `EAGAIN` retry followed by acquisition;
- unexpected raw futex error handling;
- ordinary contended `FUTEX_WAIT` acquisition;
- caller `errno` preservation and destroy-state cleanup.

A separate freestanding probe uses real mini-libc threads. It recursively acquires
and releases a recursive mutex, exercises recursive+timed self-acquisition,
requires a worker to fail when unlocking a mutex owned by main, and holds a timed
mutex while another worker waits to a future TIME_UTC deadline and really times
out in the kernel. The probe is included in host-libc-independence inspection.

The pinned tiny-c integration performs the same public recursive, wrong-owner,
expired-deadline, and real future-timeout behavior through code emitted by the
pinned compiler. That executable is linked and run with both GNU `ld` and the
pinned mini-elf-toolchain.

## Phase boundary and promotion

Typed ownership, recursive depth, absolute timed acquisition, wrong-owner
unlock detection, and their cross-toolchain evidence close the mutex-type phase.
Additional mutex-result variants remain out of scope.

The next thread-lifecycle phase named here has now shipped: `once_flag` /
`call_once`, thread-specific storage, generation-safe key reuse, and destructor
execution on normal and explicit thread exit are documented in
`docs/once-tss.md`.

With both synchronization and TSS lifecycle in place, the next larger thread
architecture frontier is compiler-native C11 TLS interoperability: reconciling
compiler-emitted `_Thread_local` objects with mini-libc's `%fs`-based TCB on main
and cloned threads, then exposing the standard `thread_local` surface. The small
remaining `thrd_yield` scheduler primitive belongs in that conformance closure
rather than as another mutex-focused phase.
