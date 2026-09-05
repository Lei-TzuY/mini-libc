# Thread-aware runtime foundation

mini-libc now has a bounded C11 thread baseline for Linux x86-64. This phase is
an executable runtime foundation rather than a header-only `<threads.h>` surface:
mini-libc creates real kernel threads, joins them through the kernel clear-TID
lifecycle, provides a futex-backed plain mutex, and moves the existing `errno`
lvalue onto per-thread runtime state for mini-libc-created threads.

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

The implementation deliberately does not call `malloc`. It owns a fixed table
of 16 joinable thread-control slots and gives each child a 1 MiB stack obtained
directly from the existing raw `mmap` boundary. This avoids pretending the
currently unsynchronized heap allocator is already safe to use concurrently.

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

This first ABI therefore supports at most 16 simultaneously outstanding
mini-libc-created joinable threads. The fixed bound is intentional and is not a
claim of an unbounded production pthread implementation.

## Manual TCB and per-thread errno

The runtime now establishes a tiny thread-control block (TCB) without relying on
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

`errno.c` now has a replaceable private storage provider. Hosted differential
tests, which start under the host libc rather than mini-libc `_start`, never
install that provider and therefore keep using the historical process fallback;
mini-libc does not overwrite the host runtime's FS base. Freestanding startup
installs the TCB-backed provider before entering user code, so each mini-libc
thread resolves the same source-level `errno` lvalue to its own TCB slot.

This is the first previously process-global libc state made genuinely
thread-specific by the new runtime foundation. It does not imply that every
other mini-libc subsystem is thread-safe.

## Mutex and futex contract

A plain `mtx_t` is currently one integer state word. Lock acquisition uses an
x86 memory `xchg`, which is an atomic full barrier for the shared state. A
contended thread sleeps with raw `futex(FUTEX_WAIT)` and unlock exchanges the
word back to zero before waking one waiter with `FUTEX_WAKE`.

`mtx_trylock` deterministically reports `thrd_busy` when the state is already
held. Unlocking an unlocked mini-libc mutex is rejected with `thrd_error`.
Ownership tracking, recursive locking, timed waits, priority protocols, and
robust-mutex recovery are not part of this baseline.

## Executable evidence

The freestanding thread probe runs real concurrent kernel threads and verifies:

- the main and child `thrd_current` identities are distinct;
- two workers each begin with an independent zero-valued `errno` slot while the
  parent keeps a pre-existing `EIO` value;
- the workers assign distinct errno values and return them through `thrd_join`
  without changing the parent's errno;
- two workers perform 8,000 total increments through the same plain mutex and
  produce the exact protected counter value;
- `mtx_trylock` reports busy while the mutex is held;
- an explicit `thrd_exit(73)` is observed as join result 73;
- a repeated join and an unlock of an unlocked mutex are rejected;
- the final marker is written only after all children have been joined.

A second freestanding termination probe keeps a child thread alive, waits until
that child is ready, and calls `_Exit(37)` from the main thread. The child is
armed to emit a `survived` marker after a bounded futex wait. Correct
`SYS_exit_group` behavior must terminate the whole process with status 37 before
that marker can appear; the previous single-thread-era `SYS_exit` behavior would
leave the child running and is therefore observably rejected.

The pinned tiny-c integration independently creates two real threads, executes
3,000 mutex-protected increments, checks parent/child errno isolation, joins an
explicit `thrd_exit(91)`, and emits `tiny-threads-ok`. The same object graph is
linked and run through both GNU `ld` and the pinned mini-elf-toolchain. The
freestanding thread probes and tiny integration are also included in host-libc
independence inspection.

## Phase boundary and promotion

This phase establishes thread creation/lifecycle, one synchronization primitive,
and a real per-thread libc state slot. It does **not** make mini-libc globally
thread-safe. In particular, the brk-based allocator, FILE registry/buffering,
termination callback registries, environment storage, and hidden string state
remain unsynchronized unless their individual contracts say otherwise.

The next architectural frontier is **synchronized shared-runtime state**, with
the heap allocator the highest-value first target. A coherent follow-on should
make `malloc`/`calloc`/`realloc`/`free` safe under concurrent mini-libc threads,
prove concurrent allocation/free integrity with deterministic executable stress,
and then use that foundation to remove fixed thread-control limits or support
higher-level TSS/condition-variable facilities. Adding more `<threads.h>` names
without first making shared runtime ownership safe would not satisfy that
promotion.
