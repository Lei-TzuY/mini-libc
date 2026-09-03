# mini-libc

A small, correctness-focused C runtime and libc laboratory for x86-64 Linux.
The long-term goal is to provide a progressively usable userspace runtime for
`tiny-c-compiler`, `mini-elf-toolchain`, and eventually `minios-x86`, without
trying to recreate glibc.

## Current milestone: runtime plus memory primitives

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

The first standard-library surface is now present in `string.h`:
`memcpy`, `memmove`, `memset`, and `memcmp`. The implementation is deliberately
byte-oriented rather than optimized so overlap direction, unsigned-byte
comparison, return values, and zero-length behavior stay easy to audit.

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
status, direct syscall behavior, mmap/munmap, deterministic memory edge cases,
and fixed-seed randomized memory cases. It also builds a separate hosted
`memory_differential` executable that recompiles the production memory source
under test-only symbol names and compares it against the host libc. That hosted
oracle is test-only; `memory_probe` remains a freestanding mini-libc executable.

`make inspect` rejects a `PT_INTERP`, dynamic `NEEDED` entries, or unresolved
symbols in every freestanding milestone executable, including `memory_probe`.

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

The next bounded layer is the string core (`strlen`, `strcmp`, `strncmp`,
`strcpy`, `strncpy`, `strchr`, and `strrchr`) with deterministic edge cases and
host differential tests. Cross-repository integration will wait until mini-libc
is stable on the system assembler/linker bootstrap path.
