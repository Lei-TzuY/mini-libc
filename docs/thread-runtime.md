# Thread-aware runtime foundation

mini-libc now has an executable C11 thread/runtime baseline for Linux x86-64.
It creates real kernel threads, joins and detaches them through the kernel
clear-TID lifecycle, provides futex-backed plain mutexes and condition variables,
installs per-thread `errno` state, and serializes the shared brk allocator so
distinct threads can allocate and free concurrently.

This phase is not a header-only `<threads.h>` surface. Thread creation, lifetime
ownership, detach reclamation, synchronization, TLS setup, allocator interaction,
and process-versus-thread termination all have freestanding and pinned
cross-toolchain executable evidence.

## Public surface

`<threads.h>` currently exposes:

- `thrd_t` and `thrd_start_t`;
- `thrd_create`, `thrd_detach`, `thrd_join`, `thrd_current`, `thrd_equal`, and
  `thrd_exit`;
- `mtx_t` plus `mtx_init`, `mtx_lock`, `mtx_trylock`, `mtx_unlock`, and
  `mtx_destroy`;
- `cnd_t` plus `cnd_init`, `cnd_wait`, `cnd_signal`, `cnd_broadcast`, and
  `cnd_destroy`;
- `thrd_success`, `thrd_nomem`, `thrd_timedout`, `thrd_busy`, `thrd_error`, and
  `mtx_plain` result/type constants.

Only plain mutexes and untimed condition waits are implemented. Recursive/timed
mutexes, `cnd_timedwait`, `thrd_sleep`, `thrd_yield`, `once_flag`/`call_once`, and
the TSS APIs are outside this phase.

## Dynamic thread controls and publication

The old fixed table of 16 joinable thread-control slots has been removed. A user
thread now owns a dynamically allocated control object plus a 1 MiB stack mapped
through the raw `mmap` boundary. The synchronized allocator introduced before
this phase is therefore part of the thread-lifetime dependency graph rather than
an unavailable resource that forces a static control table.

Every live user thread control participates in one private process-global linked
registry protected by a small atomic/futex lock. The control records the kernel
TID, kernel clear-TID word, result, lifecycle state, stack mapping, start routine,
argument, and manual TCB. The registry itself is private; no pointer to a control
object is part of the public `thrd_t` ABI.

A newly allocated control is inserted into the registry before the raw `clone`
call but starts as unpublished. This ordering is required because the child can
run immediately and may even call `thrd_detach(thrd_current())` before the parent
returns from `clone`. The child reaches its control through the TCB, so
self-detach does not require a published TID lookup. The detached reaper is not
allowed to reclaim an unpublished control. After `clone` returns successfully,
the parent records the returned TID and publishes the control under the registry
lock, then wakes the reaper if the child already detached itself.

This publication barrier prevents a fast self-detaching child from freeing its
control while the parent is still completing `thrd_create`. On clone failure,
the unpublished registry entry is removed, the stack is unmapped, and the
control is freed.

There is no artificial fixed mini-libc thread-count ceiling in this phase.
Creation can still fail because the allocator, `mmap`, or kernel refuses the
resource request; removing the old 16-slot table is not a claim of unlimited
threads.

## Join and detach ownership

A user control has one synchronized lifecycle owner. The important states are:

```text
JOINABLE
   |-- thrd_join   --> JOINING  --> clear-TID --> unmap --> unlink --> free
   |
   `-- thrd_detach --> DETACHED --> clear-TID --> REAPING --> unmap --> unlink --> free
```

The registry lock serializes the `JOINABLE` transition, so concurrent join and
detach attempts cannot both acquire ownership. A second join, a second detach,
or a join after a successful detach is rejected with `thrd_error`.

`thrd_join` claims `JOINABLE -> JOINING`, waits on the kernel-managed child
clear-TID word with `futex`, reads the integer result, unmaps the child stack,
unlinks the control, and frees it. An unexpected futex failure or stack-unmap
failure restores the control to `JOINABLE` so the caller can retry rather than
silently losing the only lifetime owner.

`thrd_detach` also supports a child detaching itself. A target that has already
reached kernel clear-TID zero can be reclaimed synchronously. A still-running
target becomes `DETACHED` and is owned by the private reaper described below.
All public thread-lifecycle calls preserve the caller's pre-existing `errno`;
allocation and kernel failures are reported through the `thrd_*` result contract
rather than leaking an internal temporary errno value.

## Detached-thread reaper

A running detached thread cannot safely unmap its own active stack, and its
control object must stay alive until the kernel has finished using the
`CLONE_CHILD_CLEARTID` address during thread exit. mini-libc therefore uses one
lazy internal reaper thread rather than freeing detached resources in the child.

The first detach of a still-running target starts this internal service. The
reaper has one process-lifetime 1 MiB stack and a static private control; it is
not present in the public user-thread registry and cannot be joined or detached
through a user `thrd_t` handle. Program termination through `SYS_exit_group`
reclaims the reaper's own process-lifetime mapping.

For user detached controls, reclamation is strictly ordered after kernel
clear-TID completion. The reaper scans the synchronized registry, claims only
published `DETACHED` controls whose `clear_tid` is zero, transitions them to
`REAPING`, then unmaps the dead thread's stack, unlinks the control, and frees it.
A failed unmap restores `DETACHED` so a later reaper pass can retry. This makes
user stack/control reclamation exactly-once with respect to join/detach ownership
rather than relying on scheduler timing.

When a detached target is still running, the reaper waits on that target's
clear-TID futex with a short bounded poll interval. The timeout prevents one
long-running detached target from indefinitely hiding a different target that
has already exited. When there is no detached target to inspect, the reaper
sleeps on a private event futex and detach/publication wakes it.

## Clone and process/thread termination

The x86-64 assembly entry issues raw Linux `clone` with shared VM/filesystem/file
descriptor/signal-handler state, `CLONE_THREAD`, `CLONE_SYSVSEM`,
`CLONE_SETTLS`, and child-set/clear-TID flags. The child starts on its mapped
stack, receives its control object, calls the C11 start routine, and funnels a
normal function return through `thrd_exit`.

`thrd_exit` stores the result in the current control and uses raw Linux
`SYS_exit`, terminating only that thread in the group. Process termination is
deliberately distinct: `_Exit`, and therefore the final step of normal `exit`,
`quick_exit`, and return from `main`, uses `SYS_exit_group` so sibling threads do
not survive C process termination.

## Manual TCB and per-thread errno

The runtime establishes a tiny thread-control block without compiler-generated
ELF TLS relocations. The private TCB begins with a self pointer followed by the
owning thread-control pointer and an `errno` slot.

At freestanding startup, before user `main`, mini-libc initializes a main TCB and
installs it as the x86-64 FS base with raw `arch_prctl(ARCH_SET_FS, ...)`. Child
creation passes its own TCB through the raw clone `CLONE_SETTLS` argument. A tiny
assembly accessor reads `%fs:0` to recover the current TCB.

The public `errno` macro remains:

```c
#define errno (*__mini_errno_location())
```

Hosted differential tests never install the mini-libc FS provider and therefore
keep the historical process fallback. Freestanding startup installs the
TCB-backed provider, so each mini-libc-created thread resolves the same
source-level errno lvalue to its own slot.

## Mutex, condition-variable, and futex contract

A plain `mtx_t` is one integer state word. Lock acquisition uses the standalone
private x86 `xchg` primitive, which is an atomic full barrier. A contended thread
sleeps with raw `futex(FUTEX_WAIT)` and unlock exchanges the word back to zero
before waking one waiter with `FUTEX_WAKE`.

A `cnd_t` is one integer generation word. `cnd_signal` atomically increments the
generation and wakes one futex waiter; `cnd_broadcast` increments the same
generation and wakes all waiters. The generation increment uses a private x86
`lock xadd` primitive shared with the existing atomic support instead of compiler
atomic builtins or a heap-backed waiter list.

`cnd_wait` atomically observes the current generation through the same primitive,
releases the caller's mutex, waits on the old generation with raw
`FUTEX_WAIT`, and reacquires the mutex before returning. The critical
release-to-sleep race is closed by the futex expected-value check: if a signaler
increments the generation after the mutex is released but before the waiter
enters the kernel sleep, the wait returns `EAGAIN` instead of blocking on a wake
that already happened. `EINTR` retries the same generation; ordinary futex wake
or an `EAGAIN` generation mismatch both complete the wait path. Unexpected futex
failures still reacquire the mutex before returning `thrd_error`.

Condition signaling is not a sticky token mechanism and spurious wakes are
allowed. Correct callers protect a predicate with the associated mutex and loop
around `cnd_wait`. Destroying a mutex or condition variable while another thread
is using it remains outside valid usage. The public condition calls preserve the
caller's pre-existing `errno` in the same style as the thread/mutex surface.

The atomic primitives remain separate archive objects shared by mutex,
condition-variable, allocator, and thread-registry synchronization paths without
forcing programs that only use `malloc` to pull clone/thread-lifecycle code into
their static image.

`mtx_trylock` reports `thrd_busy` when the state is already held. Unlocking an
unlocked mini-libc mutex is rejected with `thrd_error`. Ownership tracking,
recursive locking, timed waits, priority protocols, and robust-mutex recovery
remain outside the current contract.

## Synchronized allocator ownership

`malloc`, `free`, and the in-place metadata portion of `realloc` serialize
allocator-global state behind one private futex-backed lock. The protected state
includes the block list, free/split/coalescing transitions, tail tracking, heap
initialization, and every `brk` growth decision. `calloc` inherits the ownership
boundary through `malloc`.

The allocator lock is deliberately private rather than implemented with public
`mtx_t`, avoiding a circular dependency from allocation back into thread
creation. This synchronization is what makes dynamically allocated thread
controls safe to introduce in the current phase.

## Executable evidence

The freestanding thread probe keeps the earlier errno, mutex, explicit-exit, and
concurrent allocator regressions and adds lifecycle stress that would fail under
the previous fixed-table implementation:

- 24 user threads are created while blocked on one gate before any are joined,
  proving more than 16 simultaneously outstanding controls are real executable
  state rather than sequential slot reuse;
- all 24 are later joined and return distinct expected results;
- 24 additional running targets are detached while blocked, after which joining
  a detached handle and detaching it again are both rejected;
- every detached worker is released and observed to finish before the test
  continues, with waits bounded so an internal failure cannot hang CI forever;
- a joiner and a detacher race for one blocked target; exactly one lifecycle
  claim succeeds, and the target completes before its test synchronization
  object is destroyed;
- a child immediately calls `thrd_detach(thrd_current())`, exercising the
  pre-publication self-detach path; the parent observes successful self-detach
  and a subsequent join rejection;
- the heap remains usable after the dynamic/detached lifecycle stress;
- the earlier four-worker allocator stress still performs 2,400 concurrent
  allocation/resize/free cycles while preserving independent per-thread errno.

A separate freestanding condition probe creates six waiters behind one mutex and
condition predicate. All six report readiness through a second condition. The
main thread grants one token and calls `cnd_signal`, requires exactly one worker
to complete, then grants the remaining tokens and calls `cnd_broadcast`, after
which every worker must finish and join with its distinct expected result. Worker
and main errno sentinels remain unchanged across condition calls.

The hosted deterministic condition harness does not rely on scheduler timing. It
injects a generation increment from the fake mutex unlock exactly after
`cnd_wait` takes its snapshot and before the fake futex wait executes. The wait
must use the old expected generation, observe `EAGAIN`, reacquire the mutex, and
return success. The same harness covers interrupted-wait retry, unexpected futex
errors with mandatory mutex reacquisition, unlock/relock failures, wake failures,
signal-versus-broadcast wake counts, null arguments, and errno preservation.

The existing process-termination probe continues to keep a child alive while the
main thread calls `_Exit(37)` and requires that no child `survived` marker can
appear, proving process-wide `SYS_exit_group` behavior remains intact.

The pinned tiny-c integration independently creates 18 simultaneously blocked
user threads before joining any of them, crossing the historical 16-slot limit
through code produced by the pinned compiler. It then creates and detaches eight
running targets, checks join/second-detach rejection, waits for all workers to
finish, performs a post-stress heap sanity allocation, and still joins an
explicit `thrd_exit(91)` target.

A separate tiny-c condition executable creates four condition waiters, waits for
all of them through a progress condition, releases exactly one with `cnd_signal`,
then releases the rest with `cnd_broadcast` and joins every worker. The same
condition executable is linked and run through both GNU `ld` and the pinned
mini-elf-toolchain. Host-libc-independence inspection includes the condition
probe and pinned integration executable.

## Phase boundary and promotion

Thread creation, dynamic join/detach lifecycle, exactly-once detached user
resource reclamation, a plain mutex, untimed condition-variable synchronization,
per-thread errno, and concurrent heap ownership are now executable runtime
capabilities. This still does **not** make mini-libc globally thread-safe. FILE
registry/buffering, termination callback registries, environment storage, and
hidden string state remain unsynchronized unless their individual contracts say
otherwise.

The next architectural frontier is **timed blocking synchronization**, not more
untimed condition variants. A coherent follow-on should build on the existing
raw time/futex substrate to add an explicit absolute-time condition wait contract
and thread sleep, then decide the timed-mutex surface without weakening the plain
mutex semantics. Timeout-vs-signal races, expired deadlines, interruption, and
mutex reacquisition must have deterministic executable coverage. Once timed
blocking is real, `call_once`/TSS or broader shared-runtime synchronization such
as FILE/registry locking can be promoted as separate phases.
