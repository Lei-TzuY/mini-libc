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

`<unistd.h>` exposes `read`, `write`, `close`, `dup`, `dup2`, `pipe`, `pipe2`,
`getcwd`, `chdir`, `fchdir`, `lseek`, `ftruncate`, `unlink`, and `unlinkat`,
plus the
standard descriptor and seek constants used by those calls. `<stdio.h>`
retains ISO C `remove`/`rename` and exposes POSIX `renameat`; all pathname
mutation routes through one shared runtime. `getcwd/chdir/fchdir` add explicit
process working-directory state to the same relative-path namespace model.

`<fcntl.h>` exposes `open`, `openat`, and a bounded descriptor-control
`fcntl` surface. Supported commands are `F_DUPFD`, `F_GETFD`, `F_SETFD`, and
`F_GETFL`, with `FD_CLOEXEC` for descriptor-local close-on-exec state. Path
opening retains `AT_FDCWD`, `AT_REMOVEDIR`, and the bounded flag set:
`O_RDONLY`, `O_WRONLY`, `O_RDWR`, `O_ACCMODE`, `O_CREAT`, `O_EXCL`,
`O_TRUNC`, `O_APPEND`, `O_DIRECTORY`, and `O_CLOEXEC`.

This is intentionally not a claim that every POSIX/Linux open or fcntl
command is present.

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

Pipe IPC uses native x86-64 `pipe2` syscall 293 as the single creation
boundary; `pipe()` is the zero-flag form. `O_NONBLOCK` and `O_CLOEXEC` are
passed to the kernel atomically. Nonblocking empty reads report `EAGAIN`, while
read returns zero only after every write descriptor referring to the pipe has
been closed. Closing all readers causes a writer to receive `SIGPIPE`; when
that signal is ignored, the public `write` path reports `EPIPE`.

Current-directory state uses native x86-64 `getcwd` (79), `chdir` (80), and
`fchdir` (81). `chdir` changes the process-relative pathname base, while
`fchdir` rebinds it to an already-open directory descriptor. The public
`getcwd` wrapper returns the caller buffer on success and rejects null/zero
buffers with the bounded `EINVAL` contract; kernel size failures surface as
`ERANGE`.

Descriptor duplication/control uses native x86-64 `dup` (32), `dup2` (33),
and `fcntl` (72). Duplicated descriptors share one open-file description, so
file offset and file status flags are shared while `FD_CLOEXEC` remains
descriptor-local. `dup2` retains native replacement and same-fd semantics.
The bounded `fcntl` wrapper consumes a variadic argument only for `F_DUPFD`
and `F_SETFD`; unmodeled commands fail with `EINVAL`.

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
then verifies `opendir/readdir` observe `.`, `..`, two named regular files,
a subdirectory, and at least 180 deterministic filler entries with coherent
inode/type/name records. The filler set intentionally exceeds the 4 KiB stream
buffer so both the first traversal and the post-`rewinddir` traversal require
multiple `getdents64` refills. The probe correlates `dirfd` metadata with
pathname metadata, proves EOF preserves `errno`, checks missing-path `ENOENT`
and non-directory `ENOTDIR`, and cleans the owned entries so the harness can
remove the root directory after removing the filler set. The same executable
runs through pinned tiny-c-compiler and both GNU ld and mini-elf-toolchain.

`tests/descriptor_control_probe.c` proves the Unix descriptor model with real
files. A descriptor and its `dup` share read offset; `F_GETFL` reports the
same open-file status flags on both. `F_SETFD` changes `FD_CLOEXEC` only on
the selected descriptor, while `F_DUPFD` creates a descriptor at or above the
requested minimum with close-on-exec cleared and the same shared offset.
`dup2` replaces an existing descriptor with the source open-file description
without changing the replaced descriptor's pathname, and same-fd `dup2`
returns the descriptor unchanged. The probe also locks `EBADF` and bounded
command `EINVAL` behavior and runs through pinned tiny-c-compiler plus GNU ld
and mini-elf-toolchain.

`tests/cwd_state_probe.c` saves the original cwd as both a pathname and an open
directory descriptor, changes into a harness-created tree, and correlates the
new cwd with pathname metadata by device/inode. Relative `open`, `stat`, and
`opendir` then create/read/enumerate namespace state beneath the new cwd. The
probe descends into a child directory, accesses the parent through `..`, proves
missing/non-directory `chdir` errors leave cwd unchanged, locks bad-fd
`fchdir`, and finally restores the exact original cwd through the saved
descriptor. It also exercises `getcwd` success, short-buffer `ERANGE`, bounded
null/zero-size `EINVAL`, and successful-call errno preservation. The same
executable runs through pinned tiny-c-compiler plus GNU ld and
mini-elf-toolchain.

`tests/pipe_ipc_probe.c` proves both blocking-default and nonblocking pipe
semantics. Plain `pipe` transfers bytes and reaches EOF after the writer closes.
`pipe2(O_NONBLOCK|O_CLOEXEC)` proves descriptor-local close-on-exec state,
shared nonblocking file status, empty-read `EAGAIN`, and data transfer. A
duplicated writer keeps EOF suppressed after the original writer closes; once
the final duplicate closes, read returns zero. The probe then ignores
`SIGPIPE`, closes the last reader, and verifies write fails with `EPIPE`.
Unsupported pipe2 flags return `EINVAL`. The same executable runs through
pinned tiny-c-compiler plus GNU ld and mini-elf-toolchain.

## Next architectural promotion

Descriptor opening/I/O, pathname lifecycle, file sizing, and metadata now form
one executable filesystem baseline. `tests/metadata_probe.c` verifies the
native 144-byte stat ABI against real regular files and directories, correlates
pathname and descriptor metadata by device/inode, and proves both shrinking
and extending files through `ftruncate`, including zero-filled growth.
Bad descriptors, negative lengths, and missing paths exercise
`EBADF`/`EINVAL`/`ENOENT` boundaries. The same probe runs through pinned
tiny-c-compiler and both GNU ld and mini-elf-toolchain.

Descriptor I/O, filesystem namespace state, and pipe IPC now form one
executable Unix runtime baseline. The next coherent promotion is **descriptor
readiness**: a bounded `poll` surface that can prove readable/writable/hangup
transitions over real pipes without timing guesses or blocking races. Further
pipe work should require a genuinely new capability such as atomic flag/control
semantics, not additional wrapper variants. Higher-level stdio should continue
sharing this syscall substrate rather than growing a second kernel ABI.
