# mini-libc

A small, correctness-focused C runtime and libc laboratory for x86-64 Linux.
The long-term goal is to provide a progressively usable userspace runtime for
`tiny-c-compiler`, `mini-elf-toolchain`, and eventually `minios-x86`, without
trying to recreate glibc.

## Current milestone: runtime, memory/string core, integer conversion, and errno

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
`mini_sys_*`: failures return negative kernel errno values directly. Raw syscall
wrappers do not yet translate failures to `-1` or update libc `errno`.

The standard `string.h` surface includes the memory primitives `memcpy`,
`memmove`, `memset`, and `memcmp`, plus `strlen`, `strcmp`, `strncmp`, `strcpy`,
`strncpy`, `strchr`, and `strrchr`. Implementations stay deliberately simple so
overlap direction, unsigned-byte comparisons, termination/padding semantics,
and search behavior remain easy to audit.

The current `stdlib.h` surface contains `atoi` and `strtol`. `atoi` skips the six
C whitespace characters, accepts one optional `+` or `-`, consumes decimal
digits until the first non-digit, and returns zero when no digits are consumed.
ISO C does not define `atoi` overflow behavior; mini-libc chooses a deterministic
policy instead: positive overflow saturates to `INT_MAX` and negative overflow
to `INT_MIN`, without relying on signed-overflow undefined behavior.

`strtol` accepts base 0 or bases 2 through 36, handles the standard octal and
hexadecimal prefixes, reports the first unconsumed character through `endptr`,
and leaves `errno` unchanged when no range/base error occurs. With no conversion,
`endptr` points back to the original input. Positive/negative range errors return
`LONG_MAX`/`LONG_MIN` and set `ERANGE`. Unsupported bases follow the targeted
Linux/glibc contract: return zero, set `EINVAL`, and leave a non-null `endptr`
slot untouched. The implementation consumes the full valid digit sequence even
after range overflow so `endptr` remains correct.

The `errno.h` surface defines Linux `EINVAL` as 22 and `ERANGE` as 34 and
provides the standard modifiable `errno` lvalue through
`__mini_errno_location()`. The backing slot is process-global and zero-initialized,
not thread-local; mini-libc does not have TLS setup yet. The accessor indirection
keeps the source-level `errno` contract stable when TLS support arrives.

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
status, direct syscall behavior, mmap/munmap, deterministic memory/string/integer
conversion edge cases, fixed-seed randomized cases, and the errno lvalue/storage
contract. Separate hosted differential executables recompile the production
memory/string/atoi/strtol sources under test-only symbol names and compare them
against the host libc where the target contract is comparable. Those hosted
oracles are test-only; `memory_probe`, `string_probe`, `atoi_probe`,
`errno_probe`, and `strtol_probe` remain freestanding mini-libc executables.

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
src/errno/           errno storage boundary
tests/               freestanding probes, differential tests, ELF checks
examples/            freestanding sample programs
docs/                ABI contracts and design notes
```

Standard headers are added only as their required surface becomes real. The
current `stddef.h` provides `size_t`, `string.h` declares only implemented
memory/string routines, `stdlib.h` declares only `atoi` and `strtol`, and
`errno.h` currently provides the errno lvalue contract plus `EINVAL` and
`ERANGE`.

See [`docs/abi.md`](docs/abi.md) for the exact ABI assumptions, raw syscall
contract, and current errno storage limitation.

## Next

The next bounded conversion slice can implement `strtoul` separately, reusing
the now-tested base/prefix/end-pointer model while validating unsigned overflow
and leading-minus semantics independently. Cross-repository integration will
wait until mini-libc is stable on the system assembler/linker bootstrap path.
