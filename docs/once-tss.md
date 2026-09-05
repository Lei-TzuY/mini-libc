# Once-only initialization and thread-specific storage

This document records the C11 thread-lifecycle phase that follows typed/timed
mutexes. It adds process-wide once-only initialization and bounded
thread-specific storage directly to the mini-libc Linux x86-64 thread runtime.
The implementation is allocation-free after thread creation and does not depend
on host pthread or host TLS services.

## Public surface

`<threads.h>` now exposes:

```c
typedef struct {
    int __state;
} once_flag;

#define ONCE_FLAG_INIT {0}

void call_once(once_flag *flag, void (*func)(void));

typedef unsigned int tss_t;
typedef void (*tss_dtor_t)(void *);

#define TSS_DTOR_ITERATIONS 4

int tss_create(tss_t *key, tss_dtor_t dtor);
void tss_delete(tss_t key);
void *tss_get(tss_t key);
int tss_set(tss_t key, void *value);
```

The runtime supports 32 simultaneously active TSS keys. Exhausting that bounded
registry returns `thrd_nomem`. Successful and failed TSS operations preserve the
caller's pre-existing `errno`; the `thrd_*` result remains the public failure
channel where one exists.

## Exactly-once state machine

`call_once` uses three flag states:

```text
0  uninitialized
1  initializer running
2  initialization complete
```

A small process-wide transition lock serializes only state inspection and state
publication. The initializer itself runs outside that lock, so initialization
functions attached to different flags may execute concurrently.

The first caller changes `0 -> 1`, releases the transition lock, invokes the
initializer, then publishes `2` and wakes all futex waiters on the flag. A caller
that observes state 1 sleeps with `FUTEX_WAIT(flag, 1)` and retries. If completion
races with entry into the wait, the futex expected-value comparison returns
immediately and the caller observes state 2 on the next loop.

The locked transition boundaries provide the publication barrier used by this
runtime: callers do not return from `call_once` after observing completion until
they have passed through the same atomic lock path that follows the initializer's
writes.

This phase does not define recovery when an initializer terminates its thread or
otherwise fails to return normally. In that case the flag remains in the running
state; the public C11 API has no error result with which to publish a failed
initialization.

## TSS key identity and reuse

The key registry has 32 slots. A public `tss_t` token encodes both a slot index
and a nonzero generation. Deleting a key invalidates its registry slot; creating
a later key in that slot increments the generation before publishing the new
token.

Each thread control block stores, for every slot:

- one `void *` value;
- the generation for which that value was installed.

`tss_get` returns a value only when both the public token and the current
thread's stored generation match the currently active registry generation.
Consequently, deleting and recreating a slot cannot make another still-live
thread's old value visible through the new key. The implementation uses a
27-bit generation field in the current 32-bit token layout; wraparound is a
bounded implementation limit rather than an unbounded ABA claim.

`tss_set` validates the active generation before updating only the current TCB.
`tss_delete` invalidates the key globally and clears a matching value in the
calling thread, but it never invokes a destructor. Values that remain in other
TCBs after deletion are unreachable because their generation is stale.

## Destructor lifecycle

Worker TCBs now own the fixed TSS value/generation arrays. New worker controls
explicitly zero those arrays before clone publication; the main and reaper TCBs
have static-storage zero initialization.

`thrd_exit` is the single TSS destructor hook. This means both paths:

```text
thread start function returns
    -> __mini_thread_run
    -> thrd_exit

explicit thrd_exit
```

run TSS destructors before the raw kernel thread exit and before Linux clears the
join futex word. A successful `thrd_join` therefore cannot observe the thread as
finished while its TSS destructors are still running.

For each destructor pass, the runtime snapshots all active matching non-null TSS
values, clears those TCB values, drops the TSS registry lock, then calls the
snapshot destructors. Clearing before invocation permits a destructor to install
a new value with `tss_set`; that value is picked up on the next pass.

At most `TSS_DTOR_ITERATIONS` (currently 4) passes are executed. A destructor
that re-arms its key on the fourth pass may therefore leave a non-null value
behind when thread termination proceeds, as permitted by the bounded destructor
iteration contract.

This phase intentionally does not claim that normal process `exit` runs TSS
destructors for the main thread. TSS destruction is attached to C11 thread
termination through `thrd_exit`, not to the process-wide termination stack.

## Executable evidence

The freestanding `once_tss_probe` runs real mini-libc threads and proves:

- eight workers rendezvous and contend on one `once_flag`;
- the initializer executes exactly once and publishes one shared value;
- every worker begins with a null value for the same TSS key;
- every worker reads back only its own TSS value;
- normal thread return runs a destructor that re-arms the key for three passes;
- an explicit `thrd_exit(77)` also runs the same destructor lifecycle before
  join observes result 77;
- a later `call_once` does not rerun the initializer;
- public TSS calls preserve the caller's existing `errno`;
- the final executable remains host-libc independent.

The deterministic hosted harness additionally proves:

- completed and waiter-side `call_once` state transitions;
- 32-key capacity and `thrd_nomem` on exhaustion;
- TSS isolation between two synthetic TCBs;
- delete/recreate produces a different token and hides stale values;
- invalid/deleted `tss_set` fails without changing caller `errno`;
- destructor re-arming stops after exactly `TSS_DTOR_ITERATIONS` passes;
- null-destructor values are not cleared by the destructor runner;
- `tss_delete` does not invoke a destructor.

The pinned tiny-c thread integration compiles and executes `call_once`, `tss_*`,
normal-return destructors, and explicit-`thrd_exit` destructors in the existing
thread stress executable. The same executable is linked and run through both GNU
`ld` and the pinned mini-elf-toolchain.

## Phase boundary and promotion

Exactly-once initialization, bounded generation-safe TSS, and thread-exit
destructor passes close this C11 thread-lifecycle phase. More key-count variants
or additional destructor-pass tests are not the next priority.

The next larger thread-runtime frontier is compiler-native C11 TLS
interoperability. The pinned tiny-c compiler already emits real `_Thread_local`
objects using the x86-64 local-exec TLS model, while mini-libc currently owns the
`%fs` thread-pointer base for its custom TCB. Before claiming `<threads.h>`
`thread_local` interoperability, the runtime and linker path need an explicit
contract that makes compiler-emitted TLS objects coexist with the mini-libc TCB
on main and cloned threads. That phase should include real `_Thread_local`
isolation through GCC, Clang, tiny-c, and mini-elf. The remaining `thrd_yield`
scheduler boundary is a small C11 conformance item to close alongside that
larger TLS integration, not a standalone feature milestone.
