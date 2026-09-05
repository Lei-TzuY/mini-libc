# Pathname lifecycle contract

This phase extends mini-libc's owned `FILE` support from pathname-backed stream
creation into an executable pathname-management workflow. The implementation is
still intentionally Linux x86-64 specific and uses the raw syscall boundary
rather than pretending to provide a general POSIX portability layer.

## Exclusive `fopen` creation

`fopen` accepts the C11 `x` mode character only for `w`-family modes. It may
appear once alongside `+` and `b` in any order after the initial `w`, so forms
such as `wx`, `w+x`, and `w+bx` are accepted. `rx`, `ax`, duplicate `x`, and
other unknown/duplicate mode characters are rejected with `EINVAL` before a
`FILE` object is allocated or a raw open is issued.

A `w...x` mode maps to the existing create/truncate flags plus Linux `O_EXCL`.
The kernel therefore performs the create-vs-existing decision atomically. If the
target already exists, `fopen` returns null with `EEXIST`; the existing file is
not truncated. Successful exclusive streams otherwise use the same owned FILE,
buffering, registry, positioning, flushing, and close lifecycle as ordinary
pathname-backed streams.

`<errno.h>` exposes Linux x86-64 `EEXIST = 17`, and `strerror(EEXIST)` returns
`"File exists"` without disturbing the caller's existing `errno` value.

## Rename and remove

`<stdio.h>` exposes the ISO C pathname functions:

```c
int rename(const char *oldname, const char *newname);
int remove(const char *filename);
```

`rename` rejects a null path with `EINVAL`. Otherwise it invokes raw
`renameat(AT_FDCWD, oldname, AT_FDCWD, newname)`. Kernel failures are translated
from negative raw errno values to `-1` plus positive libc `errno`; success
returns zero and preserves the incoming `errno`. Replacement and cross-filesystem
behavior are whatever the Linux `renameat` operation reports; mini-libc does not
add a second userspace copy/replace path.

`remove` rejects a null path with `EINVAL`. It first attempts
`unlinkat(AT_FDCWD, filename, 0)`. If Linux reports `EISDIR`, it retries the same
pathname with `AT_REMOVEDIR`, so the public API covers ordinary files and empty
directories through one deterministic path. Other kernel failures map directly
to positive libc `errno`; success returns zero without clearing a prior errno.

## Raw syscall boundary

The raw layer adds:

| API | x86-64 Linux syscall |
| --- | ---: |
| `mini_sys_unlinkat` | 263 |
| `mini_sys_renameat` | 264 |

The four-argument `renameat` wrapper moves the fourth C argument from `rcx` to
`r10` before `syscall`, matching the Linux x86-64 syscall ABI. These raw wrappers
continue the repository-wide convention: successful values and negative kernel
errno values are returned unchanged, and raw calls never update libc `errno`.

## Executable evidence

The real freestanding file-stream probe performs an atomic publish sequence:

1. create a new file through `w+bx`;
2. write and close it;
3. attempt `wx` on the existing pathname and require `EEXIST` while the original
   contents remain unchanged;
4. rename the pathname and verify the old path disappears while the new path
   retains the data;
5. remove the renamed file and verify a second removal reports `ENOENT`.

The same probe keeps the pre-existing owned-stream, configurable-buffering,
positioning, formatted I/O, and anonymous `tmpfile()` regressions active.

Pinned tiny-c builds a separate pathname integration executable that performs
exclusive create, failed re-create, rename, read-back, and remove. The integration
script also creates an empty directory and requires that tiny-c-produced code
remove it, exercising the `EISDIR -> AT_REMOVEDIR` fallback. The same executable
is linked and run once with GNU `ld` and once through the pinned mini-elf toolchain,
and both binaries are checked for host-libc independence and leftover filesystem
state.

## Phase boundary

This phase does not add directory enumeration, metadata/stat APIs, permissions,
working-directory mutation, symbolic links, or a general POSIX filesystem
namespace. It establishes a bounded standard-C pathname lifecycle on top of the
existing FILE/openat layer: atomic exclusive creation, publication by rename,
and cleanup by remove.
