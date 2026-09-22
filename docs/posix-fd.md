# POSIX file-descriptor runtime

mini-libc now exposes a bounded errno-aware POSIX file-descriptor layer on top
of the existing raw Linux x86-64 syscall boundary.

## Public surface

`<sys/types.h>` provides the ABI types used by this layer. In addition to
`ssize_t`, `off_t`, and `mode_t`, the filesystem metadata surface exposes the
x86-64-width `dev_t`, `ino_t`, `nlink_t`, `uid_t`, `gid_t`, `blksize_t`, and
`blkcnt_t` aliases required by `struct stat`.

`<sys/stat.h>` defines a 144-byte `struct stat` whose field order and widths
match the native Linux x86-64 UAPI layout. It exposes the standard file-type
and permission masks plus `S_IS*` predicates, together with public `stat` and
`fstat` entry points.

`<unistd.h>` exposes `read`, `write`, `close`, `lseek`, `ftruncate`, `unlink`,
and `unlinkat`, plus the standard descriptor and seek constants used by those
calls. `<stdio.h>` retains ISO C `remove`/`rename` and exposes POSIX
`renameat`; all pathname mutation routes through one shared runtime.

`<fcntl.h>` exposes `open` and `openat`, `AT_FDCWD`, `AT_REMOVEDIR`, and the
bounded flag set already used by mini-libc's stdio runtime:
`O_RDONLY`, `O_WRONLY`, `O_RDWR`, `O_ACCMODE`, `O_CREAT`, `O_EXCL`,
`O_TRUNC`, and `O_APPEND`.

This is intentionally not a claim that every POSIX/Linux open flag is present.

`<dirent.h>` adds buffered directory traversal with opaque `DIR` streams,
`opendir`, `readdir`, `closedir`, `rewinddir`, and `dirfd`. The public
`struct dirent` contains inode, offset, record length, Linux d_type, and a
bounded 256-byte name. Raw variable-length Linux `getdents64` records never
escape the implementation; they are validated and copied out of a private
stream buffer.

## Runtime boundary

The public wrappers reuse `mini_sys_read`, `mini_sys_write`,
`mini_sys_close`, `mini_sys_lseek`, and `mini_sys_openat`. Raw wrappers
continue to expose Linux kernel return values unchanged; the public POSIX layer
translates a negative raw result into the conventional `-1` return and stores
the positive Linux error number in thread-aware `errno`.

Successful calls do not overwrite an existing `errno` value.

Directory streams reuse the same descriptor and allocator substrate.
`opendir` opens with `O_DIRECTORY|O_CLOEXEC`, allocates one buffered stream,
and `readdir` refills it with native x86-64 `getdents64` syscall 217.
End-of-directory returns a null pointer without changing `errno`; syscall or
record-validation failures set `errno`. `rewinddir` seeks the directory
descriptor back to offset zero and invalidates the buffered records, while
`dirfd` exposes the owned descriptor without transferring ownership.

`open` is implemented through the same `openat(AT_FDCWD, ...)` boundary, so
pathname opening has one kernel contract. A mode argument is consumed only when
`O_CREAT` is present in this bounded flag set. `stat` is implemented through
native x86-64 `newfstatat(AT_FDCWD, ..., 0)`, while `fstat` and `ftruncate`
use their native descriptor syscalls. These wrappers share the same negative
kernel return to `-1` plus thread-aware `errno` translation and leave `errno`
untouched on success. Likewise, `unlink` maps to
`unlinkat(AT_FDCWD, ..., 0)` and `rename` maps to `renameat(AT_FDCWD, ...,
AT_FDCWD, ...)`. `remove` shares the same raw unlinkat helper but preserves its
ISO C file-or-empty-directory behavior by retrying an `EISDIR` result with
`AT_REMOVEDIR`.

## Executable evidence

`tests/posix_fd_probe.c` creates a real file with
`O_CREAT|O_EXCL|O_RDWR`, writes and seeks through the public descriptor APIs,
reopens through `openat(AT_FDCWD,...)`, proves `O_TRUNC` and `O_APPEND`
against final file contents, and verifies successful-call errno preservation.
It also locks down `EBADF`, `EEXIST`, `ENOENT`, and invalid-whence
`EINVAL` translation.

`tests/posix_path_probe.c` adds real dirfd-relative lifecycle evidence: it
creates a file beneath an opened directory, renames it with `renameat`, opens
and verifies the renamed file, deletes it with `unlinkat`, checks `EBADF` for an
invalid relative dirfd, proves plain `unlink` rejects directories with
`EISDIR`, removes an empty directory with `AT_REMOVEDIR`, and verifies ISO C
`remove` on a second empty directory. Successful mutation preserves existing
`errno`, and the probe leaves no filesystem state behind.

Both POSIX probes are part of normal freestanding `make test` and `make inspect`,
and are separately compiled by pinned tiny-c-compiler then linked/executed
through both GNU ld and mini-elf-toolchain.

`tests/dirent_probe.c` builds a real namespace beneath a directory descriptor,
then verifies `opendir/readdir` observe `.`, `..`, two regular files, and a
subdirectory with coherent inode/type/name records. It correlates `dirfd`
metadata with pathname metadata, proves EOF preserves `errno`, rewinds and
enumerates the same namespace again, checks missing-path `ENOENT` and
non-directory `ENOTDIR`, and cleans every child so the harness can remove the
root directory. The same executable runs through pinned tiny-c-compiler and
both GNU ld and mini-elf-toolchain.

## Next architectural promotion

Descriptor opening/I/O, pathname lifecycle, file sizing, and metadata now form
one executable filesystem baseline. `tests/metadata_probe.c` verifies the
native 144-byte stat ABI against real regular files and directories, correlates
pathname and descriptor metadata by device/inode, and proves both shrinking
and extending files through `ftruncate`, including zero-filled growth.
Bad descriptors, negative lengths, and missing paths exercise
`EBADF`/`EINVAL`/`ENOENT` boundaries. The same probe runs through pinned
tiny-c-compiler and both GNU ld and mini-elf-toolchain.

Descriptor I/O, pathname lifecycle, metadata/sizing, and directory traversal now
form one executable filesystem namespace baseline. The next coherent promotion
should move to **descriptor duplication and control**: `dup`, `dup2`, and a
bounded `fcntl` surface that proves shared open-file-description offsets,
descriptor-local close-on-exec flags, replacement semantics, and errno
boundaries. Further directory work should wait for a genuinely new capability
such as seek/tell directory positions or mutation APIs, not readdir variants.
Higher-level stdio should continue sharing this syscall substrate rather than
growing a second kernel ABI.
