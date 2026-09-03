# mini-libc

A small, correctness-focused C runtime and libc laboratory for x86-64 Linux.
The long-term goal is to provide a progressively usable userspace runtime for
`tiny-c-compiler`, `mini-elf-toolchain`, and eventually `minios-x86`, without
trying to recreate glibc.

## Current milestone: runtime, core libc primitives, integer conversion,
allocation, environment access, and write-only stdio

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
`mini_sys_*` and exposes Linux kernel return conventions directly. Most failures
are negative errno values, while raw `brk` returns the resulting program break
and reports refusal by returning the unchanged break. Raw syscall wrappers do
not update libc `errno`.

The standard `string.h` surface includes the memory primitives `memcpy`,
`memmove`, `memset`, and `memcmp`, plus `strlen`, `strcmp`, `strncmp`, `strcpy`,
`strncpy`, `strcat`, `strncat`, `strchr`, `strrchr`, `strstr`, `strspn`,
`strcspn`, and `strpbrk`.
`strstr` returns the first matching substring and treats an empty needle as a
match at the haystack start. `strspn` counts the initial bytes present in an
accept set, while `strcspn` counts the initial bytes absent from a reject set.
`strpbrk` returns the first byte in the source that belongs to an accept set.
`strcat` appends a complete non-overlapping source string at the destination
terminator and returns the original destination pointer. `strncat` follows the
same return and non-overlap contract while appending at most the requested
number of source bytes and always writing a final null terminator. The searches
and scans compare byte representations directly without allocating or copying.
Implementations stay deliberately simple so overlap direction, unsigned-byte
comparisons, termination/padding semantics, and search behavior remain easy to
audit.

The current `stdlib.h` integer-conversion surface contains `atoi`, `strtol`, and
`strtoul`. `atoi` skips the six C whitespace characters, accepts one optional
`+` or `-`, consumes decimal digits until the first non-digit, and returns zero
when no digits are consumed. ISO C does not define `atoi` overflow behavior;
mini-libc chooses a deterministic policy instead: positive overflow saturates
to `INT_MAX` and negative overflow to `INT_MIN`, without relying on signed-
overflow undefined behavior.

`strtol` accepts base 0 or bases 2 through 36, handles the standard octal and
hexadecimal prefixes, reports the first unconsumed character through `endptr`,
and leaves `errno` unchanged when no range/base error occurs. With no conversion,
`endptr` points back to the original input. Positive/negative range errors return
`LONG_MAX`/`LONG_MIN` and set `ERANGE`. Unsupported bases follow the targeted
Linux/glibc contract: return zero, set `EINVAL`, and leave a non-null `endptr`
slot untouched. The implementation consumes the full valid digit sequence even
after range overflow so `endptr` remains correct.

`strtoul` uses the same base, prefix, `endptr`, invalid-base, and errno model.
A magnitude above `ULONG_MAX` returns `ULONG_MAX` and sets `ERANGE`. A leading
minus is accepted by the C conversion contract: when the magnitude is
representable, the result is its unsigned negation modulo `ULONG_MAX + 1`; for
example, `strtoul("-1", ..., 10)` returns `ULONG_MAX` without setting `ERANGE`.

`getenv` retains the original `envp` vector decoded at process startup and
scans it without allocating or copying. A lookup matches only an exact
`NAME=` prefix, so `FOO` does not match `FOOBAR`. An empty environment value
returns a non-null pointer to its terminating null byte. Missing names, empty
lookup names, and lookup names containing `=` return null. Successful and
unsuccessful lookups leave `errno` unchanged. The returned value pointer
aliases the process environment storage. mini-libc does not currently expose
the POSIX `environ` global or environment mutation APIs.

The minimal `stdio.h` surface provides `EOF`, `putchar`, and `puts` as
unbuffered write-only standard-output operations. `putchar` writes the
`unsigned char` conversion of its argument and returns that byte as an `int` on
success. `puts` writes the complete string followed by a newline and returns a
nonnegative value on success. Both routines retry positive short writes, map a
negative raw `write` result to libc `errno`, and return `EOF` on failure. A
zero-progress write is treated as `EIO` so the retry loop cannot stall forever.
Successful calls leave an existing `errno` value unchanged. `FILE`, `stdout`,
buffering, formatted I/O, and input routines are not exposed yet.

The allocator surface now provides `malloc`, `calloc`, `realloc`, and `free`.
It is deliberately a small single-threaded x86-64 allocator backed only by the
raw `brk` boundary.
Returned payloads are 16-byte aligned. `malloc(0)` returns null without changing
`errno`; overflow or a refused heap-growth request returns null and sets
`ENOMEM`. `calloc` uses the same allocator, returns null without changing
`errno` when either dimension is zero, checks `nmemb * size` before multiplying,
and zero-initializes every byte of successful allocations. `realloc(NULL, n)`
uses `malloc(n)`. For a non-null pointer, zero size frees the allocation and
returns null without changing `errno`. Nonzero resize preserves exactly the
minimum of the old requested size and the new requested size: shrinking stays
in place when possible, growth first consumes an adjacent free block or extends
the allocator tail with `brk`, and otherwise moves through allocate/copy/free.
A failed resize returns null, sets `ENOMEM` when allocation fails, and leaves the
original allocation and its contents intact. `free(NULL)` is a no-op. Freed
blocks are reused with first-fit search, split when a useful aligned remainder
exists, and coalesced with adjacent free blocks. The allocator does not currently
return tail space to the kernel, is not thread-safe, and must own the program
break once it has initialized; callers must not move the break directly while
allocator state is live. As in C, invalid-pointer and double-free calls are
outside the supported contract.

The `errno.h` surface defines Linux `EIO` as 5, `ENOMEM` as 12, `EINVAL` as 22,
and `ERANGE` as 34 and provides the standard modifiable `errno` lvalue through
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
conversion, string-copy/bounded-concatenation, search, membership-scan, and
counting-scan edge cases,
allocator alignment/reuse/split/coalescing behavior,
`calloc` zeroing/overflow semantics, `realloc` in-place/move/failure semantics,
fixed-seed allocation/resize stress, startup-backed `getenv`
exact-match/empty/missing semantics, write-only stdio success/short-write/error
behavior, and the errno lvalue/storage contract.
Separate hosted differential executables compare the production
memory/string/conversion sources against host libc where the target contract is
comparable. A test-only fake-`brk` allocator harness deterministically
verifies heap-growth refusal and `ENOMEM` without linking the freestanding
allocator to the host heap. Hosted oracles are test-only; all library probes,
including `allocator_probe`, remain freestanding mini-libc executables.

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
src/stdlib/          conversion, allocation, and environment utilities
src/stdio/           unbuffered write-only standard I/O
src/errno/           errno storage boundary
tests/               freestanding probes, differential tests, ELF checks
examples/            freestanding sample programs
docs/                ABI contracts and design notes
```

Standard headers are added only as their required surface becomes real. The
current `stddef.h` provides `size_t`, `string.h` declares only implemented
memory/string routines including `strcat`, `strncat`, `strstr`, `strspn`,
`strcspn`, and `strpbrk`;
`stdlib.h` declares `atoi`, `strtol`, `strtoul`,
`getenv`, `malloc`, `calloc`, `realloc`, and `free`; `stdio.h` provides `EOF`,
`putchar`, and `puts`; and `errno.h` currently provides the errno lvalue
contract plus `EIO`, `ENOMEM`, `EINVAL`, and `ERANGE`.

See [`docs/abi.md`](docs/abi.md) for the exact ABI assumptions, raw syscall
contract, allocator ownership rules, and current errno storage limitation.

## Next

With bounded concatenation in place, the next useful standard C memory slice can
add `memchr` with zero-count, first-match, null-byte, high-byte, and no-match
regression coverage. Formatted I/O, buffering, `FILE`, input routines,
environment mutation, threading, and mmap-backed large allocations should remain
separate later slices. Cross-repository integration will wait until mini-libc is
stable on the system assembler/linker bootstrap path.
