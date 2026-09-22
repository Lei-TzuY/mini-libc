# POSIX file-descriptor runtime

mini-libc now exposes a bounded errno-aware POSIX file-descriptor layer on top
of the existing raw Linux x86-64 syscall boundary.

## Public surface

`<sys/types.h>` provides the ABI types used by this layer:

- `ssize_t` and `off_t` are signed 64-bit `long` values on the x86-64 target.
- `mode_t` is an unsigned 32-bit integer.

`<unistd.h>` exposes `read`, `write`, `close`, and `lseek`, plus the
standard descriptor and seek constants used by those calls.

`<fcntl.h>` exposes `open` and `openat`, `AT_FDCWD`, and the bounded flag
set already used by mini-libc's stdio runtime:
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
`O_CREAT` is present in this bounded flag set.

## Executable evidence

`tests/posix_fd_probe.c` creates a real file with
`O_CREAT|O_EXCL|O_RDWR`, writes and seeks through the public descriptor APIs,
reopens through `openat(AT_FDCWD,...)`, proves `O_TRUNC` and `O_APPEND`
against final file contents, and verifies successful-call errno preservation.
It also locks down `EBADF`, `EEXIST`, `ENOENT`, and invalid-whence
`EINVAL` translation.

The probe is part of normal freestanding `make test` and `make inspect`, and
is separately compiled by pinned tiny-c-compiler then linked/executed through
both GNU ld and mini-elf-toolchain.

## Next architectural promotion

The next coherent POSIX filesystem slice should build on this descriptor layer
rather than add unrelated constants: public pathname mutation and descriptor
metadata/sizing operations such as `unlink`/`unlinkat`, `rename`/
`renameat`, `ftruncate`, and a bounded `stat`/ `fstat` contract once the
required kernel structures are modeled and executable. Higher-level stdio
should continue sharing the same raw syscall substrate rather than growing a
second kernel ABI.
