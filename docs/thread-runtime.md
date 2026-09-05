# Thread-aware runtime foundation

mini-libc now has a bounded C11 thread baseline for Linux x86-64. This phase is
an executable runtime foundation rather than a header-only `<threads.h>` surface:
mini-libc creates real kernel threads, joins them through the kernel clear-TID
lifecycle, provides a futex-backed plain mutex, moves the existing `errno`
lvalue onto per-thread runtime state for mini-libc-created threads, and now
serializes the shared brk allocator so distinct threads can safely allocate and
free concurrently.

## Public surface

`<threads.h>` currently exposes:

- `thrd_t` and `thrd_start_t`;
- `thrd_create`, `thrd_join`, `thrd_current`, `thrd_equal`, and `thrd_exit`;
- `mtx_t` plus `mtx_init`, `mtx_lock`, `mtx_trylock`, `mtx_unlock`, and
  `mtx_destroy`;
- `thrd_success`, `thrd_nomem`, `thrd_timedout`, `thrd_busy`, `thrd_error`, and
  `mtx_plain` result/type constants.

Only plain mutexes are implemented. Recursive/timed mutexes, `thrd_detach`,
`thrd_sleep`, `thrd_yield`, condition variables, `once_flag`/`call_once`, and the
TSS APIs are outside this phase.

## Thread creation and lifetime

The current thread-creation implementation still owns a fixed table of 16
joinable thread-control slots and gives each child a 1 MiB stack obtained
directly from the existing raw `mmap` boundary. The original thread foundation
deliberately avoided `malloc` because the heap allocator was then process-global
and unsynchronized. The allocator is now safe for concurrent mini-libc calls,
but thread controls remain fixed-size until a dedicated lifetime/detach phase
replaces that table with dynamically owned control objects.

The x86-64 assembly entry issues raw Linux `clone` with shared VM/filesystem/file
descriptor/signal-handler state, `CLONE_THREAD`, `CLONE_SYSVSEM`,
`CLONE_SETTLS`, and child-set/clear-TID flags. The child starts on the new mmap
stack, receives its control slot, calls the C11 start routine, and funnels a
normal function return through `thrd_exit`.

`thrd_join` waits on the kernel-managed child clear-TID word with `futex`. Once
the kernel has cleared the word and woken waiters, the parent reads the child's
integer result, unmaps its stack, and releases the fixed control slot. A second
join of the released handle is rejected. `thrd_exit` stores the result in the
current control slot and uses raw Linux `SYS_exit`, which terminates only the
calling thread in the thread group.

Process termination is deliberately distinct. `_Exit`, and therefore the final
step of normal `exit`, `quick_exit`, and return from `main`, uses raw
`SYS_exit_group`. Once real C11 threads exist, using `SYS_exit` for those paths
would strand sibling threads instead of terminating the program. The raw syscall
layer therefore exposes both operations with separate names rather than hiding
this Linux lifecycle distinction.

This ABI still supports at most 16 simultaneously outstanding mini-libc-created
joinable threads. The bound remains explicit and is not a claim of an unbounded
production pthread implementation.

## Manual TCB and per-thread errno

The runtime establishes a tiny thread-control block (TCB) without relying on
compiler-generated ELF TLS relocations. The private TCB begins with a self
pointer followed by the owning thread-control pointer and an `errno` slot.

At freestanding process startup, before user `main`, mini-libc initializes a main
TCB and installs it as the x86-64 FS base with raw
`arch_prctl(ARCH_SET_FS, ...)`. Child creation passes its own TCB through the
raw clone `CLONE_SETTLS` argument. A tiny assembly accessor reads `%fs:0` to
recover the current TCB.

The public `errno` macro remains unchanged:

```c
#define errno (*__mini_errno_location())
```

`errno.c` has a replaceable private storage provider. Hosted differential tests,
which start under the host libc rather than mini-libc `_start`, never install
that provider and therefore keep using the historical process fallback;
mini-libc does not overwrite the host runtime's FS base. Freestanding startup
installs the TCB-backed provider before entering user code, so each mini-libc
thread resolves the same source-level `errno` lvalue to its own TCB slot.

This is the first previously process-global libc state made genuinely
thread-specific by the runtime foundation. It does not imply that every other
mini-libc subsystem is thread-safe.

## Mutex and futex contract

A plain `mtx_t` is currently one integer state word. Lock acquisition uses an
x86 memory `xchg`, which is an atomic full barrier for the shared state. A
contended thread sleeps with raw `futex(FUTEX_WAIT)` and unlock exchanges the
word back to zero before waking one waiter with `FUTEX_WAKE`.

The atomic exchange primitive is now a standalone private assembly object rather
than living inside the clone-entry object. This preserves static-archive
granularity: allocator synchronization can reuse the same primitive without
pulling thread creation, TLS setup, or clone lifecycle code into a program that
only calls `malloc`.

`mtx_trylock` deterministically reports `thrd_busy` when the state is already
held. Unlocking an unlocked mini-libc mutex is rejected with `thrd_error`.
Ownership tracking, recursive locking, timed waits, priority protocols, and
robust-mutex recovery are not part of this baseline.

## Synchronized allocator ownership

`malloc`, `free`, and the in-place metadata portion of `realloc` now serialize
all allocator-global state behind one private futex-backed lock. The protected
state includes the block list, free/split/coalescing transitions, tail tracking,
heap initialization, and every `brk` growth decision. The allocator algorithm is
otherwise unchanged; this phase is a correctness boundary, not a new allocation
strategy or a scalability claim.

The lock is deliberately private rather than implemented with public `mtx_t`.
That keeps the allocator dependent only on the small atomic/futex substrate and
avoids a circular architectural dependency from `malloc` back into thread
creation. Raw futex failures do not allow an unlocked metadata path: acquisition
always retries the atomic exchange before entering the allocator critical
section.

`realloc` holds the allocator lock while it evaluates and performs shrink,
adjacent-free-block growth, or tail-`brk` growth. If none of those in-place paths
can satisfy the request, it records the old requested size, releases the lock,
and only then performs the public `malloc` -> copy -> `free` fallback. This
avoids recursive acquisition of the allocator lock while keeping the original
allocation live until replacement succeeds. A failed replacement therefore
still leaves the caller's old allocation intact.

`calloc` inherits the allocator ownership guarantee through `malloc`; once a
block is returned, only that caller owns its payload while `calloc` zeroes it.
Successful allocation paths preserve the calling thread's existing `errno`, and
allocation/range failure continues to report `ENOMEM` through the thread-local
errno slot.

The current lock is coarse grained and wakes a futex waiter on release. No
throughput, fairness, lock-free, or fine-grained allocator claim is made by this
phase.

## Executable evidence

The freestanding thread probe runs real concurrent kernel threads and verifies:

- the main and child `thrd_current` identities are distinct;
- two workers each begin with an independent zero-valued `errno` slot while the
  parent keeps a pre-existing `EIO` value;
- the workers assign distinct errno values and return them through `thrd_join`
  without changing the parent's errno;
- two workers perform 8,000 total increments through the same plain mutex and
  produce the exact protected counter value;
- four additional workers concurrently execute 2,400 total allocator cycles
  without an external mutex, mixing non-aligned `malloc`, zero-checked `calloc`,
  growing and shrinking `realloc`, byte-pattern preservation checks, and `free`;
- all allocator workers retain independent per-thread errno values, and the main
  thread can allocate, fill, verify, and free a 4 KiB block after the stress;
- `mtx_trylock` reports busy while the mutex is held;
- an explicit `thrd_exit(73)` is observed as join result 73;
- a repeated join and an unlock of an unlocked mutex are rejected;
- the final marker is written only after all children have been joined.

The hosted allocator failure harness keeps the existing ENOMEM, realloc
failure-atomicity, adjacent reuse, tail growth, and coalescing checks. It also
forces two apparent lock-contention observations and returns raw `EAGAIN` from
the fake futex wait; the allocator must retry acquisition, complete the
allocation, and preserve the caller's errno instead of entering metadata
unlocked or surfacing the raw synchronization result.

A second freestanding termination probe keeps a child thread alive, waits until
that child is ready, and calls `_Exit(37)` from the main thread. The child is
armed to emit a `survived` marker after a bounded futex wait. Correct
`SYS_exit_group` behavior must terminate the whole process with status 37 before
that marker can appear; the previous single-thread-era `SYS_exit` behavior would
leave the child running and is therefore observably rejected.

The pinned tiny-c integration independently creates two real threads. Each
worker now performs 1,500 allocator cycles with `malloc`, periodic `calloc`,
content-preserving `realloc`, and `free` while also executing the existing
mutex-protected counter and errno-isolation checks. The parent verifies the
3,000-cycle result, performs a post-stress heap sanity allocation, joins an
explicit `thrd_exit(91)`, and emits `tiny-threads-ok`. The same object graph,
including the standalone atomic primitive, is linked and run through both GNU
`ld` and the pinned mini-elf-toolchain. The freestanding thread probes and tiny
integration remain included in host-libc independence inspection.

## Phase boundary and promotion

Thread creation/lifecycle, a plain mutex, per-thread errno, and concurrent heap
ownership are now executable runtime capabilities. This still does **not** make
mini-libc globally thread-safe. FILE registry/buffering, termination callback
registries, environment storage, and hidden string state remain unsynchronized
unless their individual contracts say otherwise.

The allocator prerequisite that forced the original fixed 16-slot thread-control
table is now removed. The next architectural frontier is **dynamic thread-control
ownership and detach lifecycle**: allocate joinable controls dynamically, make
the registry/lifetime transition itself synchronized, remove the fixed slot
ceiling, and add `thrd_detach` with exactly-once stack/control reclamation after
kernel clear-TID completion. That lifecycle should be proven under concurrent
create/join/detach stress before promoting further to condition variables, TSS,
or broader shared-runtime synchronization.
