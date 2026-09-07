# Public C11 atomics interoperability

This document records mini-libc's bounded public C11 atomic interoperability
baseline for static x86-64 Linux executables and the first internal
synchronization-convergence slice built on top of it. Atomics remain a
compiler-language boundary: mini-libc supplies a stable `<stdatomic.h>` entry
point and executable runtime integration, while the selected compiler owns
machine-level atomic lowering and memory-order semantics.

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

The public atomic boundary is now also used by the first set of runtime state
machines instead of remaining a standalone language-surface feature.

`once_flag`, `mtx_t`, and `cnd_t` use real C11 atomic storage in `<threads.h>`.
Compile-time x86-64 ABI guards require the atomic integer/unsigned-long storage
to retain the existing futex and owner-word sizes, and require `once_flag`,
`cnd_t`, and `mtx_t` to retain their established public object sizes. Mutex
state, owner identity, and recursion depth now use C11 atomic load/store,
exchange, and fetch-add operations. Condition sequence publication uses C11
atomic load/store/fetch-add. `call_once` uses C11 atomic storage for both the
public flag and its private transition serializer.

The migration deliberately keeps default sequentially consistent operations for
this slice. The retired private x86 helpers were exchange/xadd based and already
provided conservative ordering. Changing memory-order policy at the same time as
changing the abstraction would make it harder to distinguish an ownership or
futex regression from an ordering optimization.

The futex ABI remains explicit. Atomic state words used by the thread APIs are
required to remain lock-free 32-bit words on this supported target and their
addresses are passed to the raw futex boundary after the same representation
checks that protect the public object ABI.

As a result, the following generic private assembly entry points have been
removed and are forbidden from reappearing in production by the compiler-
neutrality/source audit:

- `__mini_atomic_fetch_add_int`;
- `__mini_atomic_load_ulong`;
- `__mini_atomic_exchange_ulong`.

The hosted mutex target also no longer links `atomic.o`, providing a build-graph
check that the migrated mutex state machine is independent of the remaining
private assembly object.

## Remaining private synchronization boundary

Internal convergence is not yet complete. `src/internal/atomic.S` remains in the
archive because several private runtime surfaces still use its integer exchange
primitive and because stdio still uses its specialized recursive lock:

- allocator metadata / `brk` ownership;
- thread-control registry and detached-thread reaper coordination;
- TSS registry serialization;
- the process-wide stdio serializer and its per-thread recursive depth.

Those paths are intentionally left untouched by this slice. In particular, the
stdio lock has a separate recursive/TLS contract and should not be treated as a
mechanical replacement of an integer exchange call.

## Phase boundary and promotion

This phase closes two bounded milestones for the current static x86-64 runtime:
public C11 atomic interoperability, and C11-atomic convergence of the public C11
synchronization object state machines (`mtx_t`, `cnd_t`, and `once_flag`). It
does not claim general dynamic linking, non-lock-free large atomics, 128-bit
atomics, or a portable contract for pointer fetch arithmetic.

The next higher-value synchronization frontier remains **private runtime lock
convergence**. Allocator, thread registry/reaper, TSS registry, and stdio still
depend on the remaining bespoke scalar assembly boundary. A coherent next phase
should migrate those private serializers onto the proven C11 atomic abstraction,
preserve every futex and recursive-lock invariant, retain deterministic fake-
runtime tests, and delete `src/internal/atomic.S` only when no runtime consumer
remains and GCC, Clang, tiny-c, GNU `ld`, and mini-elf all prove the converted
executable behavior.
