# mini-libc

A small, correctness-focused C runtime and libc laboratory for x86-64 Linux.
The long-term goal is to provide a progressively usable userspace runtime for
`tiny-c-compiler`, `mini-elf-toolchain`, and eventually `minios-x86`, without
trying to recreate glibc.

## Current milestone: runtime plus memory/string core

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

The standard `string.h` surface now includes the memory primitives `memcpy`,
`memmove`, `memset`, and `memcmp`, plus `strlen`, `strcmp`, `strncmp`, `strcpy`,
`strncpy`, `strchr`, and `strrchr`. Implementations stay deliberately simple so
overlap direction, unsigned-byte comparisons, termination/padding semantics,
and search behavior remain easy to audit.

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
status, direct syscall behavior, mmap/munmap, deterministic memory/string edge
cases, and fixed-seed randomized cases. Separate hosted differential executables
recompile the production memory/string sources under test-only symbol names and
compare them against the host libc. Those hosted oracles are test-only;
`memory_probe` and `string_probe` remain freestanding mini-libc executables.

`make inspect` rejects a `PT_INTERP`, dynamic `NEEDED` entries, or unresolved
symbols in every freestanding milestone executable, including both library
probes.

## Layout

```text
include/             implemented standard public headers
include/mini/        implemented project-specific public APIs
src/crt/             process entry and startup
src/syscall/         Linux x86-64 syscall boundary
src/string/          memory and string primitives
tests/               freestanding probes, differential tests, ELF checks
examples/            freestanding sample programs
docs/                ABI contracts and design notes
```

Standard headers are added only as their required surface becomes real. The
current `stddef.h` intentionally provides the `size_t` type needed by the
implemented `string.h` memory API; broader standard-header coverage remains a
later milestone.

See [`docs/abi.md`](docs/abi.md) for the exact ABI assumptions and raw syscall
contract.

## Next

The next bounded layer is basic integer conversion (`atoi`, then `strtol` and
`strtoul`) with explicit overflow and invalid-input behavior. Cross-repository
integration will wait until mini-libc is stable on the system assembler/linker
bootstrap path.
