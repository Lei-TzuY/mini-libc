# Compiler-native static TLS interoperability

This document records the thread-runtime phase that integrates compiler-emitted
C11 `_Thread_local` storage with mini-libc's existing `%fs`-based thread control
block (TCB). The implementation targets static x86-64 Linux executables using
the local-exec TLS model. It also closes the remaining `thrd_yield` scheduler
boundary in `<threads.h>`.

## Public surface

`<threads.h>` now provides the C11 spelling alias:

```c
#ifndef thread_local
#define thread_local _Thread_local
#endif
```

and:

```c
void thrd_yield(void);
```

`thrd_yield` calls the raw Linux `sched_yield(2)` boundary and restores the
caller's pre-existing libc `errno`. The API has no result value, so a raw kernel
failure is deliberately not translated into a libc error channel.

## Static TLS / TCB layout

The pinned tiny-c compiler emits x86-64 local-exec TLS references using the
thread pointer obtained from `%fs:0` plus an `R_X86_64_TPOFF32` relocation.
Mini-libc already requires `%fs:0` to contain the TCB's `self` pointer. The two
contracts therefore coexist by using the Variant-II-style layout:

```text
lower addresses

[ compiler static TLS block ] [ mini-libc TCB ]
                              ^
                              thread pointer / FS base
                              FS:0 == tcb.self == TP

higher addresses
```

The executable's static TLS image occupies negative offsets from the thread
pointer. Mini-libc-owned state such as thread-local `errno` and TSS slots remains
at non-negative offsets inside the TCB and does not alias compiler TLS objects.

This phase deliberately implements a bounded static TLS profile:

- maximum static TLS block size: 4096 bytes;
- maximum required TLS alignment: 16 bytes;
- one `PT_TLS` image per static executable;
- local-exec TLS only.

Executables whose `PT_TLS` requirements exceed those bounds are rejected during
thread-runtime initialization rather than silently mis-layouting TLS storage.
These are explicit implementation limits, not a claim of general ELF dynamic
TLS support.

## Runtime discovery

`crt0` passes the original Linux initial stack to
`__mini_thread_runtime_init_main`. Before changing `%fs`, the thread runtime
walks:

```text
argc
argv[]
NULL
envp[]
NULL
auxv[]
```

and reads `AT_PHDR`, `AT_PHENT`, and `AT_PHNUM`. The program-header table is
validated with bounded counts and checked address arithmetic, then scanned for
`PT_TLS`.

For a valid TLS header, the runtime records:

- `p_vaddr` as the initialized `.tdata` template address;
- `p_filesz` as initialized template bytes;
- `p_memsz` as total `.tdata + .tbss` bytes;
- `p_align` as the required alignment;
- `align_up(p_memsz, p_align)` as the static TLS block size.

No `PT_TLS` is a valid configuration and leaves the compiler-TLS reserve unused.
Malformed or unsupported TLS metadata fails closed during startup.

## Main and worker initialization

The main-thread storage and every worker control reserve the bounded compiler TLS
area immediately before their TCB. The reaper control uses the same layout.

Main-thread startup performs:

```text
initial stack
  -> discover PT_TLS
  -> zero static TLS block
  -> copy p_filesz bytes from the TLS template
  -> initialize mini-libc TCB
  -> arch_prctl(ARCH_SET_FS, &tcb)
  -> install thread-local errno provider
  -> user main
```

Worker creation still uses `CLONE_SETTLS` with the worker TCB as the new FS
base. In the child, before entering any C worker function, the assembly entry
obtains the new TCB from `%fs:0`, zeros the compiler TLS block, copies the
initialized TLS template, and only then enters `__mini_thread_run`.

Consequently every new worker begins with independent initialized `.tdata`,
zeroed `.tbss`, and block-scope static `_Thread_local` state. Main-thread
mutations are not inherited as the worker's initial TLS values.

## Linker contract

The compiler/linker/runtime integration relies on a real ELF TLS contract rather
than a user-space key/value emulation:

```text
C `_Thread_local`
  -> STT_TLS / .tdata / .tbss
  -> R_X86_64_TPOFF32
  -> static linker local-exec relocation
  -> PT_TLS in the executable
  -> mini-libc auxv discovery and per-thread template initialization
  -> TP-relative machine access at runtime
```

The pinned mini-elf-toolchain now supports the local-exec TPOFF relocation and
emits `PT_TLS`. The integration pin also includes two linker correctness fixes
found by this phase: zero-sized layout markers must neither split the static TLS
image nor create false `PT_LOAD` overlap failures. Genuine non-zero section
overlaps remain rejected.

## Executable evidence

The freestanding `tls_probe` is compiled with real compiler TLS objects and
proves:

- initialized file-scope `thread_local` values;
- zero-initialized TLS;
- block-scope `static thread_local` state;
- distinct TLS addresses for main and each worker;
- independent worker mutations;
- preservation of the main thread's TLS mutations across worker execution;
- thread-local libc `errno` remains isolated alongside compiler TLS;
- `thrd_yield` preserves `errno` and TLS state.

The deterministic TLS harness additionally proves:

- malformed `PT_TLS` with `p_filesz > p_memsz` is rejected;
- TLS blocks larger than 4096 bytes are rejected;
- alignments above 16 bytes are rejected;
- initialized bytes are copied and `.tbss` bytes are zeroed;
- unrelated bytes in the reserved compiler-TLS area are not overwritten;
- the compiler TLS block ends exactly at the TCB / thread pointer;
- a no-`PT_TLS` executable keeps the reserve untouched.

The pinned tiny-c integration compiles `tls_probe.c` itself, verifies that the
object contains `R_X86_64_TPOFF32`, links it first through GNU `ld` and then
through the pinned mini-elf-toolchain, verifies that the final executable has a
`PT_TLS` program header, executes it, and requires the exact `tls-ok` marker.
The resulting ELF remains host-libc independent.

This gives executable evidence across GCC, Clang, tiny-c, GNU `ld`, and
mini-elf-toolchain; it is not a synthetic thread-local array substituted for
compiler TLS.

## Phase boundary and promotion

This phase closes bounded compiler-native static local-exec TLS interoperability
for the current static x86-64 Linux runtime together with `thrd_yield`.

It does **not** claim:

- dynamic TLS / DTV allocation;
- TLS for `dlopen`-style modules;
- general-purpose unbounded static TLS allocation;
- arbitrary TLS alignments beyond the documented bound;
- ELF TLS descriptors or other dynamic TLS models;
- C++ `thread_local` constructors/destructors.

Those require a different loader/runtime architecture and should not be implied
by this bounded static-executable milestone.

The public C11 atomics interoperability frontier identified by this phase is now
closed as a bounded executable baseline. Mini-libc exposes `<stdatomic.h>`
through compiler-owned lowering for GCC/Clang and a pinned tiny-c fallback, and
proves scalar atomic operations, memory orders, fences, real relaxed contention,
and release/acquire publication across GCC, Clang, tiny-c, GNU `ld`, and
mini-elf. See `docs/atomics.md` for the exact portability boundary.

The next higher-value synchronization frontier is internal synchronization
convergence: allocator, thread lifecycle, mutex/condition, once/TSS, and stdio
still depend on bespoke scalar helpers in `src/internal/atomic.S`. A future
phase should migrate those proven state machines onto the now-established C11
atomic abstraction, preserving futex and recursive-lock invariants while
reducing private assembly only when all existing deterministic and
cross-toolchain runtime gates remain green.
