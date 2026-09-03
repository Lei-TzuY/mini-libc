# mini-libc

A small, correctness-focused C runtime and libc laboratory for x86-64 Linux.
The long-term goal is to provide a progressively usable userspace runtime for
`tiny-c-compiler`, `mini-elf-toolchain`, and eventually `minios-x86`, without
trying to recreate glibc.

## Current milestone: runtime, memory/string core, and `atoi`

The repository builds real ELF executables through this path:

```text
Linux process entry
  -> _start (mini-libc crt0)
  -> decode argc / argv / envp
  -> main(argc, argv, envp)
  -> mini_sys_exit(main_status)
  -> SYS_exit
```

`examples/hello.c` writes through `mini_sys_write`, which executes the Linux
x86-64 `write` syscall directly. No host CRT or host libc is linked into the
resulting executable.

The raw syscall layer currently implements `read`, `write`, `close`, `lseek`,
`brk`, `mmap`, `munmap`, and `exit`. Its API is intentionally named
`mini_sys_*`: failures return negative kernel errno values directly. Standard
POSIX wrappers and `errno` will be added only when their semantics can be
implemented completely.

The standard `string.h` surface includes the memory primitives `memcpy`,
`memmove`, `memset`, and `memcmp`, plus `strlen`, `strcmp`, `strncmp`, `strcpy`,
`strncpy`, `strchr`, and `strrchr`. Implementations stay deliberately simple so
overlap direction, unsigned-byte comparisons, termination/padding semantics,
and search behavior remain easy to audit.

The current `stdlib.h` surface intentionally contains only `atoi`. It skips the
six C whitespace characters, accepts one optional `+` or `-`, consumes decimal
digits until the first non-digit, and returns zero when no digits are consumed.
ISO C does not define `atoi` overflow behavior; mini-libc chooses a deterministic
policy instead: positive overflow saturates to `INT_MAX` and negative overflow
to `INT_MIN`, without relying on signed-overflow undefined behavior. Hosted
differential tests therefore compare only inputs whose results are representable
as `int`, while freestanding regressions lock the mini-libc overflow policy.

## Build and verify

Requirements: an x86-64 Linux environment with a C compiler, GNU-compatible
`ld`/`ar`, `readelf`, `nm`, and POSIX shell utilities.

```sh
make
make test
make inspect
./build/hello
```

`make test` verifies process-stack decoding, propagation of `main`'s return
status, direct syscall behavior, mmap/munmap, deterministic memory/string/atoi
edge cases, and fixed-seed randomized cases. Separate hosted differential
executables recompile the production memory/string/atoi sources under test-only
symbol names and compare them against the host libc where the C contract is
portable. Those hosted oracles are test-only; `memory_probe`, `string_probe`,
and `atoi_probe` remain freestanding mini-libc executables.

`make inspect` rejects a `PT_INTERP`, dynamic `NEEDED` entries, or unresolved
symbols in every freestanding milestone executable, including all library
probes.

## Layout

```text
include/             implemented standard public headers
include/mini/        implemented project-specific public APIs
src/crt/             process entry and startup
src/syscall/         Linux x86-64 syscall boundary
src/string/          memory and string primitives
src/stdlib/          integer conversion and later general utilities
tests/               freestanding probes, differential tests, ELF checks
examples/            freestanding sample programs
docs/                ABI contracts and design notes
```

Standard headers are added only as their required surface becomes real. The
current `stddef.h` provides `size_t`, `string.h` declares only implemented
memory/string routines, and `stdlib.h` declares only `atoi`.

See [`docs/abi.md`](docs/abi.md) for the exact ABI assumptions and raw syscall
contract.

## Next

Before exposing `strtol` and `strtoul`, mini-libc needs a minimal `errno`/`ERANGE`
contract so range errors can be implemented rather than silently omitted. After
that, the next conversion slice can add `strtol`/`strtoul` with explicit base,
end-pointer, invalid-input, and overflow tests. Cross-repository integration will
wait until mini-libc is stable on the system assembler/linker bootstrap path.
