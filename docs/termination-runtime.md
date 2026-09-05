# Termination runtime lifecycle

mini-libc now exposes three deliberately distinct C termination paths on Linux
x86-64:

```text
normal termination
  -> exit(status)
  -> normal atexit registry, LIFO
  -> flush every live writable FILE
  -> _Exit(status)
  -> SYS_exit_group

quick termination
  -> quick_exit(status)
  -> quick-exit registry, LIFO
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
The separation is part of the executable ABI contract, not merely an
implementation detail.

## Public surface

`<stdlib.h>` exposes:

```c
int atexit(void (*func)(void));
int at_quick_exit(void (*func)(void));

_Noreturn void exit(int status);
_Noreturn void quick_exit(int status);
_Noreturn void _Exit(int status);
_Noreturn void abort(void);
```

Both callback registries currently have a fixed capacity of 32 entries and are
process-global. Null callback registration is rejected deterministically with
`-1`. Capacity exhaustion also returns `-1` without disturbing previously
registered callbacks. The thread-runtime baseline makes `errno` thread-specific,
but these callback registries remain intentionally unsynchronized until a later
shared-runtime-state phase.

## Normal termination

`atexit()` records callbacks in one private registry. `exit()` removes and calls
them in reverse registration order. After the registry is drained, `exit()`
invokes the stdio live-stream flush sweep and finally delegates to `_Exit()`.
Returning from `main` follows this same path through the crt startup layer.

The normal registry is independent of the C11 quick-exit registry. A normal
return from `main` or an explicit `exit()` does not run callbacks registered by
`at_quick_exit()`.

## Quick termination

`at_quick_exit()` records callbacks in a second private registry. `quick_exit()`
drains only this registry in reverse registration order and then calls `_Exit()`.
It does not run normal `atexit()` callbacks and does not invoke the stdio flush
sweep.

Pending buffered stdio therefore remains unpublished unless the program flushed
it explicitly before calling `quick_exit()`. Quick-exit callbacks may still use
the limited mini-libc facilities available to ordinary C code, but this phase
does not add thread-safety, reentrancy, or special async-signal-safety claims.

## Immediate termination

`_Exit()` remains the minimal direct program-termination primitive. It performs
no callback execution and no stdio cleanup. In the thread-aware runtime it issues
raw Linux `SYS_exit_group`, not `SYS_exit`, so every thread in the process is
terminated. It is also the final primitive used by both normal and quick
termination once their respective lifecycle work is complete.

The distinction is now explicit in the raw syscall layer: `mini_sys_exit` maps
to `SYS_exit` and is reserved for thread-local termination such as `thrd_exit`,
while `mini_sys_exit_group` maps to `SYS_exit_group` and backs C process
termination. Before real threads existed those syscalls were observationally
equivalent for mini-libc programs; after `CLONE_THREAD` they are not.

## Abnormal termination

`abort()` is implemented in the signal object so programs that only use normal
or quick termination do not pull the signal subsystem into the static link.
`abort()` raises `SIGABRT` once, permitting a previously installed handler to
run. If control returns, it resets the disposition to `SIG_DFL` and raises
`SIGABRT` again. This makes a returning handler unable to turn `abort()` into a
normal return.

Normal and quick callback registries are not drained, and stdio is not flushed.
An emergency `_Exit(134)` is present only if the signal path unexpectedly returns
again. The public signal baseline currently has no signal-mask API, so blocked
`SIGABRT` through mechanisms outside mini-libc remains outside this C-focused
contract rather than being hidden behind a false POSIX conformance claim.

## Executable evidence

The existing freestanding runtime probe exercises the single-thread lifecycle
boundaries:

- normal return and explicit `exit()` retain reverse-order `atexit` behavior;
- `_Exit()` bypasses callbacks and buffering;
- `quick_exit()` runs three quick handlers in reverse order while ignoring three
  normal handlers and pending buffered stdout;
- quick registry capacity and null-registration failure are deterministic;
- normal return ignores a registered quick handler;
- `abort()` runs a real `SIGABRT` handler once, then terminates after restoring
  the default disposition, while normal/quick callbacks and buffered stdout stay
  invisible.

The thread-runtime phase adds a separate active-child regression: a child thread
announces readiness and waits on a futex while the main thread calls `_Exit(37)`.
The child is armed to write `survived` after a bounded wait. The test requires
process status 37 and empty output, proving `_Exit` uses process-wide
`SYS_exit_group`; the old `SYS_exit` implementation would strand the sibling
thread and expose the marker.

A hosted signal harness replaces the signal syscalls and emergency `_Exit` to
lock the exact `abort()` fallback sequence without killing the test process. The
harness uses mini-libc's own `setjmp`/`longjmp` implementation for that escape
path, so the signal and non-local-control subsystems share one context ABI rather
than mixing mini-libc headers with host `jmp_buf` storage.

Pinned tiny-c compiles a dedicated termination executable. Its quick mode proves
the independent C11 callback registry and no-flush behavior. Its abort mode
executes a real `SIGABRT` handler that returns and still ends with signal status
134. GNU `ld` and the pinned mini-elf-toolchain both link and run that executable,
and host-libc-independence inspection covers the resulting binaries. The thread
runtime independently exercises `thrd_exit` through both toolchains, so the
thread-local and process-wide raw exit paths are both executable parts of the
current runtime.

## Phase boundary and promotion

This phase completes the first C termination matrix: normal cleanup, C11 quick
termination, direct `_Exit`, and signal-backed abnormal termination. Non-local C
control transfer is an independent executable baseline documented in
`docs/setjmp-runtime.md`, and `docs/thread-runtime.md` now defines the real C11
thread lifecycle that makes the `SYS_exit`/`SYS_exit_group` distinction
architecturally significant.

The remaining limitation is no longer absence of threads; it is unsynchronized
shared runtime ownership. Callback registries, stdio global registry/buffering,
the brk-based allocator, environment storage, and other hidden process state are
not generally safe for concurrent mutation. The thread-runtime roadmap therefore
promotes next to synchronized shared state, beginning with the allocator, rather
than adding more termination variants.
