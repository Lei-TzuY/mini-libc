# Non-local control-transfer runtime

mini-libc now provides a C11-oriented `<setjmp.h>` baseline for x86-64 SysV
non-local control transfer. The implementation is deliberately small: it saves
only the execution context required to resume a live C frame and has no heap,
syscall, signal-mask, or stdio dependency.

## Public surface

`<setjmp.h>` exposes:

```c
typedef struct {
    unsigned long __rbx;
    unsigned long __rbp;
    unsigned long __r12;
    unsigned long __r13;
    unsigned long __r14;
    unsigned long __r15;
    unsigned long __rsp;
    unsigned long __rip;
} jmp_buf[1];

int setjmp(jmp_buf env);
_Noreturn void longjmp(jmp_buf env, int value);
```

The array-of-one public type keeps ordinary C parameter passing pointer-like
while making the saved state caller-owned and allocation-free. The concrete
layout is an x86-64 mini-libc ABI rather than a portable object representation;
programs must treat `jmp_buf` as opaque state even though this first header keeps
the fields visible for the tiny freestanding implementation.

## Saved SysV execution context

`setjmp` saves the six x86-64 SysV callee-saved general-purpose registers
`rbx`, `rbp`, and `r12` through `r15`. It also records the caller's post-return
stack pointer and the return program counter. No caller-saved GP/SSE registers,
floating-point environment, signal mask, TLS state, or kernel state is captured.

The initial `setjmp` return value is zero. `longjmp(env, value)` restores the
saved register image, stack pointer, and resume program counter. A zero `value`
is normalized to one as required by the C contract; any nonzero `int` is
returned unchanged from the resumed `setjmp` expression.

## C language boundary

The implementation supports the ISO C invocation contexts required for
`setjmp`; the executable probes use it as the controlling expression of a
`switch`. mini-libc does not claim compiler-specific `returns_twice` annotations
or a compiler builtin contract. Callers must continue to obey the C rules around
non-local jumps.

In particular, automatic non-`volatile` objects in the function containing the
matching `setjmp` have indeterminate values after `longjmp` if they were modified
between the two operations. The tests use global or `volatile` state where a
value must survive the jump and do not promise stronger preservation.

A `jmp_buf` remains valid only while the frame that executed its `setjmp` is
still active. Jumping to an environment whose function has already returned, or
otherwise violating the C lifetime requirements, is outside the supported
contract.

## Executable evidence

The freestanding setjmp probe covers:

- `longjmp(env, 0)` resuming as `1`;
- an explicit nonzero value (`37`) preserved exactly;
- nested outer/inner environments and cross-function unwinding;
- a dedicated assembly helper that sets `rbx`, `rbp`, and `r12`-`r15` to known
  values, deliberately clobbers every one after `setjmp`, then verifies that
  `longjmp` restores the complete callee-saved register image;
- raw output through the freestanding syscall boundary after recovery.

The hosted signal harness now links the same mini-libc setjmp object for its
`abort()` emergency-exit capture. This removes the previous accidental mixture
of a host `jmp_buf` implementation with mini-libc headers and proves that the
non-local control-transfer ABI composes with another runtime subsystem.

Pinned tiny-c compiles a separate executable that recursively descends multiple
stack frames, performs `longjmp(inner, 0)`, verifies the normalized return value,
and then jumps to an outer environment with a distinct value. GNU `ld` and the
pinned mini-elf-toolchain both link and run that executable, and
host-libc-independence inspection covers the freestanding setjmp probe and the
cross-toolchain integration products.

## Phase boundary and promotion

This phase establishes non-local C control flow without pretending to provide
POSIX signal-mask-aware jumps. `sigsetjmp`/`siglongjmp`, signal-mask snapshots,
and asynchronous-signal recovery remain outside the contract and belong to a
later signal-runtime extension.

The larger architectural limitation now visible across mini-libc is the
single-threaded, process-global runtime-state model. `errno`, stdio registries and
buffer state, callback registries, and hidden-state facilities such as `strtok`
are not TLS-backed or synchronized. The next higher-level runtime hypothesis is
therefore a **thread-aware state foundation** rather than another control-flow
variant. A future implementation phase should begin only when it can provide an
executable vertical slice—thread creation/lifecycle plus the synchronization or
TLS machinery needed to make at least one currently process-global libc state
correct across threads. Merely adding `<threads.h>` names or mutex type shells
would not satisfy that promotion.
