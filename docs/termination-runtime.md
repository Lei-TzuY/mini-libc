# Termination runtime lifecycle

mini-libc now exposes three deliberately distinct C termination paths on Linux
x86-64:

```text
normal termination
  -> exit(status)
  -> normal atexit registry, LIFO
  -> flush every live writable FILE
  -> _Exit(status)

quick termination
  -> quick_exit(status)
  -> quick-exit registry, LIFO
  -> _Exit(status)

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
process-global, matching the single-threaded state model used elsewhere in the
runtime. Null callback registration is rejected deterministically with `-1`.
Capacity exhaustion also returns `-1` without disturbing previously registered
callbacks.

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

`_Exit()` remains the minimal direct termination primitive. It performs no
callback execution and no stdio cleanup; it issues the raw process-exit syscall.
It is also the final primitive used by both normal and quick termination once
their respective lifecycle work is complete.

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

The existing freestanding runtime probe now exercises all lifecycle boundaries:

- normal return and explicit `exit()` retain reverse-order `atexit` behavior;
- `_Exit()` still bypasses callbacks and buffering;
- `quick_exit()` runs three quick handlers in reverse order while ignoring three
  normal handlers and pending buffered stdout;
- quick registry capacity and null-registration failure are deterministic;
- normal return ignores a registered quick handler;
- `abort()` runs a real `SIGABRT` handler once, then terminates after restoring
  the default disposition, while normal/quick callbacks and buffered stdout stay
  invisible.

A hosted signal harness replaces the signal syscalls and emergency `_Exit` to
lock the exact `abort()` fallback sequence without killing the test process.

Pinned tiny-c compiles a dedicated termination executable. Its quick mode proves
the independent C11 callback registry and no-flush behavior. Its abort mode
executes a real `SIGABRT` handler that returns and still ends with signal status
134. GNU `ld` and the pinned mini-elf-toolchain both link and run that executable,
and host-libc-independence inspection covers the resulting binaries.

## Phase boundary and promotion

This phase completes the first C termination matrix: normal cleanup, C11 quick
termination, direct `_Exit`, and signal-backed abnormal termination. It does not
add POSIX process APIs, signal masks, threads, cancellation, or unwind semantics.

The next architectural promotion is non-local C control transfer rather than
more termination variants. `<setjmp.h>` can add a compact x86-64 SysV execution
context that saves callee-saved registers plus stack/program-counter state and
restores it through `longjmp`. Acceptance should include the mandated
`longjmp(env, 0) -> setjmp returns 1` rule, nested recovery, preservation of
callee-saved register state, no heap/syscall dependency, and pinned
tiny-c/mini-elf execution. POSIX signal-mask-aware `sigsetjmp`/`siglongjmp` is a
separate later frontier.
