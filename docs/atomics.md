# Public C11 atomics interoperability

This document records mini-libc's bounded public C11 atomic interoperability
baseline for static x86-64 Linux executables and the internal synchronization
convergence built on top of it. Atomics remain a compiler-language boundary:
mini-libc supplies a stable `<stdatomic.h>` entry point and executable runtime
integration, while the selected compiler owns machine-level atomic lowering and
memory-order semantics.

## Header ownership

For GCC and Clang, mini-libc's `<stdatomic.h>` is a wrapper around the compiler's
next atomic header. The wrapper first imports mini-libc `<stddef.h>` so the
compiler header sees the LP64 `ptrdiff_t` and `wchar_t` typedefs it expects, then
chains to the compiler-owned implementation.

For the pinned tiny-c compiler, mini-libc integration is intentionally built
with `-nostdinc -Iinclude`. The fallback branch therefore mirrors the pinned
compiler's documented scalar atomic builtin surface directly. Its function-like
macros are kept on one physical source line because that pinned preprocessor
does not accept a multi-line invocation of those macros.

This is a scoped compiler-language boundary, not a general relaxation of the
production compiler-neutrality rule. `tests/verify-compiler-neutral-c.sh` still
rejects compiler builtins everywhere else. `include/stdatomic.h` is restricted
to the following atomic primitives only:

- `__builtin_atomic_is_lock_free`;
- `__builtin_atomic_load` / `__builtin_atomic_store`;
- `__builtin_atomic_exchange`;
- `__builtin_atomic_fetch_add` / `__builtin_atomic_fetch_sub`;
- `__builtin_atomic_fetch_and` / `__builtin_atomic_fetch_or` /
  `__builtin_atomic_fetch_xor`;
- `__builtin_atomic_compare_exchange`;
- `__builtin_atomic_thread_fence` / `__builtin_atomic_signal_fence`.

Any additional compiler builtin in the public atomic header is a CI failure.

## Public baseline

The pinned tiny-c fallback provides the standard scalar atomic aliases used by
this phase together with `atomic_flag`, the six C11 `memory_order` values,
lock-free macros, `ATOMIC_FLAG_INIT`, `ATOMIC_VAR_INIT`, and `kill_dependency`.
GCC and Clang receive their compiler-native definitions through the wrapper
path.

The executable interoperability contract covers:

- `atomic_init`, load, store, and exchange;
- strong and weak compare/exchange, including explicit success/failure orders
  and the required update of `*expected` after a mismatch;
- integer fetch add/sub and integer fetch and/or/xor;
- `atomic_flag` test-and-set / clear;
- `atomic_is_lock_free` for the tested scalar and pointer objects;
- `atomic_thread_fence` and `atomic_signal_fence`;
- explicit relaxed, acquire, release, acquire-release, and sequentially
  consistent operations exercised through the public APIs;
- atomic pointer load/store/exchange/compare-exchange;
- lock-free `_Atomic double` load/exchange/compare-exchange on the supported
  x86-64 target.

The lock-free macros used by the bounded x86-64 profile report `2` for the
covered scalar and pointer categories.

## Pointer arithmetic boundary

This phase deliberately does **not** claim cross-compiler pointer
`atomic_fetch_add` / `atomic_fetch_sub` interoperability. During implementation,
GCC and Clang system atomic headers were observed to expose different pointer
fetch-update scaling behavior through their compiler builtin paths.

Pointer interoperability is therefore proven only through load, store,
exchange, and compare/exchange. Integer fetch-update operations remain part of
the portable mini-libc contract. This limitation is explicit so the repository
does not turn one compiler's extension behavior into a false C11 portability
claim.

## Multi-thread executable evidence

The freestanding `atomic_probe` uses mini-libc's own C11 thread runtime. It
proves both atomic operation semantics and synchronization behavior:

- scalar initialization, load/store/exchange, integer fetch/update, and CAS;
- pointer exchange/CAS and floating atomic exchange/CAS;
- `atomic_flag` and both fence APIs;
- four worker threads each perform 20,000 relaxed atomic increments and the
  exact final count is required;
- a publisher writes a non-atomic payload and then performs a release store;
  the consumer waits with an acquire load and must observe that payload.

The probe emits exactly `atomics-ok`. It is linked as a freestanding static ELF
and is included in the host-libc-independence inspection set.

The pinned tiny-c integration independently compiles `tiny_atomic_integration.c`
through mini-libc's `-nostdinc` header surface. It exercises the same public
atomic language boundary, including scalar and pointer operations, `_Atomic
double`, flags/fences, a multi-worker relaxed counter, and release/acquire
publication. The resulting executable must emit exactly `tiny-atomics-ok`.

That executable is linked and run through both GNU `ld` and the pinned
mini-elf-toolchain, so the evidence covers GCC, Clang, tiny-c, mini-libc's thread
runtime, GNU `ld`, and mini-elf rather than only checking that `<stdatomic.h>` can
be included.

## Internal synchronization convergence

The public atomic boundary is also used by runtime state machines instead of
remaining a standalone language-surface feature.

`once_flag`, `mtx_t`, and `cnd_t` use real C11 atomic storage in `<threads.h>`.
Compile-time x86-64 ABI guards require the atomic integer/unsigned-long storage
to retain the existing futex and owner-word sizes, and require `once_flag`,
`cnd_t`, and `mtx_t` to retain their established public object sizes. Mutex
state, owner identity, and recursion depth use C11 atomic load/store, exchange,
and fetch-add operations. Condition sequence publication uses C11 atomic
load/store/fetch-add. `call_once` uses C11 atomic storage for both the public flag
and its private transition serializer.

The allocator metadata lock, thread-control registry/reaper serializer, TSS
registry serializer, and process-wide stdio serializer now share the proven C11
atomic/futex boundary. The non-recursive private locks use
`src/internal/futex_lock.h`; the stdio wrapper adds only the established
per-thread recursion depth stored in the mini-libc TCB before entering that same
32-bit lock-free `atomic_int` + futex protocol.

The stdio migration deliberately preserves the existing TCB layout: the
implementation-reserved 32-bit word that the old assembly reached as `%fs:20`
continues to carry recursive depth, but C now reaches it through
`__mini_thread_current_tcb()` instead of a hard-coded segment offset. Nested
formatted/scanner/file calls therefore remain recursive without acquiring the
global word again, while only the outermost unlock publishes state zero and
issues the futex wake.

The convergence deliberately keeps default sequentially consistent operations.
The retired private x86 helpers were exchange/xadd based and already provided
conservative ordering. Changing memory-order policy at the same time as changing
the abstraction would make it harder to distinguish an ownership or futex
regression from an ordering optimization.

As a result, the following generic private assembly entry points have been
removed and are forbidden from reappearing in production by the
compiler-neutrality/source audit:

- `__mini_atomic_fetch_add_int`;
- `__mini_atomic_load_ulong`;
- `__mini_atomic_exchange_ulong`;
- `__mini_atomic_exchange_int`.

The final specialized assembly entries, `__mini_stdio_lock` and
`__mini_stdio_unlock`, have now moved to `src/stdio/lock.c` as well.
`src/internal/atomic.S` is deleted and no longer appears in either the normal
archive build or the pinned tiny-c integration archive.

The real `thread_probe` supplies contention evidence for the migrated generic
private serializers: four allocator workers repeatedly run
malloc/calloc/realloc/free while thread creation, join/detach races, and the
detached-thread reaper exercise the thread registry. `once_tss_probe` runs eight
workers through TSS generation, lookup, destructor, and thread-exit behavior.
The real `stdio_thread_probe` concurrently formats records from six workers into
one stream and scans them back, exercising nested recursive stdio calls and
cross-thread serialization. A deterministic hosted stdio-lock harness separately
locks down recursion depth and the WAIT/WAKE boundary without scheduler timing.

## Phase boundary and promotion

This phase closes the bounded synchronization-convergence milestones for the
current static x86-64 runtime: public C11 atomic interoperability, C11-atomic
convergence of public C11 synchronization objects, convergence of generic
private allocator/thread/TSS serializers, and convergence of the specialized
recursive stdio serializer.

There is no remaining private scalar synchronization assembly object to farm for
another migration PR. Future atomic work should require a new capability claim
such as a broader target/ABI profile or a deliberately justified memory-order
change with executable concurrency evidence; ordinary lock-by-lock convergence
is complete. The next repository phase should therefore be selected from the
remaining live architectural gaps rather than extending this completed
serializer-convergence line.
