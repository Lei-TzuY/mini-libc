# POSIX file-descriptor runtime

mini-libc now exposes a bounded errno-aware POSIX file-descriptor layer on top
of the existing raw Linux x86-64 syscall boundary.

## Public surface

`<sys/types.h>` provides the ABI types used by this layer:

- `ssize_t` and `off_t` are signed 64-bit `long` values on the x86-64 target.
- `mode_t` is an unsigned 32-bit integer.

`<unistd.h>` exposes `read`, `write`, `close`, `lseek`, `unlink`, and
`unlinkat`, plus the standard descriptor and seek constants used by those
calls. `<stdio.h>` retains ISO C `remove`/`rename` and now exposes POSIX
`renameat`; all pathname mutation routes through one shared runtime.

`<fcntl.h>` exposes `open` and `openat`, `AT_FDCWD`, `AT_REMOVEDIR`, and the
bounded flag set already used by mini-libc's stdio runtime:
`O_RDONLY`, `O_WRONLY`, `O_RDWR`, `O_ACCMODE`, `O_CREAT`, `O_EXCL`,
`O_TRUNC`, and `O_APPEND`.

This is intentionally not a claim that every POSIX/Linux open flag is present.

## Runtime boundary

The public wrappers reuse `mini_sys_read`, `mini_sys_write`,
`mini_sys_close`, `mini_sys_lseek`, and `mini_sys_openat`. Raw wrappers
continue to expose Linux kernel return values unchanged; the public POSIX layer
translates a negative raw result into the conventional `-1` return and stores
the positive Linux error number in thread-aware `errno`.

Successful calls do not overwrite an existing `errno` value.

`open` is implemented through the same `openat(AT_FDCWD, ...)` boundary, so
pathname opening has one kernel contract. A mode argument is consumed only when
`O_CREAT` is present in this bounded flag set. Likewise, `unlink` maps to
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

## Next architectural promotion

Descriptor opening/I/O and pathname lifecycle are now one executable filesystem
baseline. The next coherent promotion is **file sizing and metadata**:
`ftruncate` plus a bounded `stat`/`fstat` ABI after the required Linux x86-64
kernel structures are modeled explicitly and checked with real regular files,
directories, sizes, modes, and errno behavior. Higher-level stdio should
continue sharing this syscall substrate rather than growing a second kernel ABI.
