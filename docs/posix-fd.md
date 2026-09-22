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

`<poll.h>` exposes native-layout `struct pollfd`, `nfds_t`, `poll`, and the
bounded readiness bits `POLLIN`, `POLLPRI`, `POLLOUT`, `POLLERR`, `POLLHUP`,
and `POLLNVAL`. This surface maps directly to the Linux x86-64 poll ABI.

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

Readiness uses native x86-64 `poll` syscall 7. The caller-owned `pollfd`
array is passed directly to the kernel; mini-libc only translates negative
syscall returns into `-1` plus thread-aware `errno`. Zero-timeout polling is
used for deterministic probes, and readiness result bits are reported through
`revents` exactly as the kernel supplies them.

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

`tests/poll_readiness_probe.c` drives a real pipe through deterministic
zero-timeout states. An empty read end is not readable, the write end reports
`POLLOUT`, written bytes transition the read end to `POLLIN`, and closing the
writer while data remains yields readable-plus-hangup before draining to a
pure `POLLHUP` state. A closed descriptor reports `POLLNVAL`, negative
descriptors are ignored with cleared `revents`, and a writer whose readers are
all closed reports `POLLERR`. `poll(NULL, 0, 0)` and successful readiness
queries preserve sentinel `errno`. The same executable runs through pinned
tiny-c-compiler plus GNU ld and mini-elf-toolchain.

`tests/process_orchestration_probe.c` establishes a bounded single-threaded
process baseline. The parent creates a pipe and forks; the child uses only
`close`, `write`, and `_Exit`, while the parent blocks in `poll`, reads the
child payload, reaps the exact pid with `waitpid`, verifies
`WIFEXITED/WEXITSTATUS`, observes EOF/hangup after child descriptor teardown,
and proves a second reap returns `ECHILD`. Both parent and child also verify
successful `fork` preserves sentinel `errno`. The same executable runs through
pinned tiny-c-compiler plus GNU ld and mini-elf-toolchain.

This is intentionally **not** a claim of multithreaded POSIX fork safety.
mini-libc contains allocator, stdio, locale, and thread-runtime locks whose
post-fork child state is not yet repaired if another thread owned a lock at
fork time. Until that architecture is addressed, the public fork contract in
this phase is the single-threaded process-orchestration baseline exercised by
the probe.

`tests/exec_transition_probe.c` closes the program-launch loop. The parent
creates a close-on-exec pipe and forks. The child duplicates the write end onto
`STDOUT_FILENO`, duplicates it again onto fixed descriptor 100, marks only fd
100 `FD_CLOEXEC`, then calls `execve` on a separately linked mini-libc child
image with explicit argv and envp. The new image re-enters through crt0/start,
validates its arguments and `getenv` state, proves fd 100 was closed with
`EBADF`, and writes through the inherited stdout descriptor before returning
exit status 37. The parent uses `poll`, `read`, and `waitpid` to validate the
new-image output and exact exit status. A missing-image `execve` failure also
locks `ENOENT`. Both the parent and child images are built and exercised
through GCC/Clang, pinned tiny-c-compiler, GNU ld, and mini-elf-toolchain.

`<spawn.h>` adds a bounded `posix_spawn` launch surface with a fixed-capacity,
allocation-free file-actions object. This phase supports ordered `addclose`
and `adddup2` actions; the action list has eight slots, reports `ENOMEM` when
full, and its APIs return error numbers without clobbering caller `errno`.
Non-null spawn attributes, `addopen`, and `posix_spawnp` are intentionally not
modeled in this slice.

`tests/posix_spawn_probe.c` validates the multithreaded launch boundary. A
background C11 thread acquires and deliberately holds a mutex while the
calling thread performs `posix_spawn`. The child does not touch inherited
libc or application locks: it applies the prebuilt file actions using raw
`close`/`dup2`, then enters raw `execve`, with raw `exit(127)` on pre-exec or
exec failure. The actions redirect a close-on-exec pipe writer onto stdout,
and the separately linked child image validates argv/envp before writing its
marker and exiting with status 44. The parent joins the still-healthy worker,
polls/reads the child output, and reaps the exact status. A missing image is
also verified to produce the documented asynchronous spawn-failure status
127. The same parent/child pair runs through pinned tiny-c-compiler, GNU ld,
and mini-elf-toolchain.

This closes the multithreaded **launch** gap without pretending to repair every
lock after arbitrary `fork()`. The public `fork()` contract still requires a
multithreaded child to stay on async-signal-safe operations until `execve`;
`posix_spawn()` makes that discipline an implementation invariant instead of
leaving it to the caller.

`getpid`, `getppid`, and `kill` add an explicit process-identity and
control layer on top of the existing fork/wait runtime. Successful identity
queries and signal delivery preserve caller `errno`; failed `kill` translates
the raw kernel error and exposes `ESRCH`. `<sys/wait.h>` now distinguishes
normal exits from signal termination through `WIFSIGNALED` and `WTERMSIG`.

`tests/process_control_probe.c` uses two pipes to avoid scheduling guesses.
The child reports `{getpid(), getppid()}` to the parent and then blocks on a
control pipe whose writer remains open. The parent proves the reported child
pid matches the `fork` result and the child's parent pid matches the parent's
own `getpid`, checks liveness with `kill(pid, 0)`, sends `SIGTERM`, and reaps
the exact child. The resulting wait status must be signal-terminated rather
than normally exited, with `WTERMSIG(status) == SIGTERM`. A Linux-impossible
pid value also locks the `ESRCH` boundary. The same executable runs through
pinned tiny-c-compiler plus GNU ld and mini-elf-toolchain.

`getpgrp`, `getpgid`, and `setpgid` extend process control into explicit
process-group topology. `getpgrp()` is the current-process `getpgid(0)` form;
successful group queries and membership changes preserve caller `errno`.
Existing `kill` and `waitpid` semantics then become group-aware through
negative pid selectors instead of a second signaling or wait subsystem.

`tests/process_group_probe.c` creates two children that report identity and
then block on a control pipe. The parent promotes the first child to a new
process-group leader, joins the second child to that group, and verifies both
memberships with `getpgid` while its own process group remains unchanged.
`kill(-pgid, 0)` proves group liveness; `kill(-pgid, SIGTERM)` terminates both
members in one operation. Two `waitpid(-pgid, ...)` calls must reap exactly
those two distinct children with `WIFSIGNALED` and `WTERMSIG == SIGTERM`, and
a third group wait returns `ECHILD`. No sleeps or scheduler timing assumptions
are used. The same executable runs through pinned tiny-c-compiler plus GNU ld
and mini-elf-toolchain.

`getsid` and `setsid` extend the process model from process groups into the
session hierarchy. Successful session queries and creation preserve caller
`errno`; `setsid` creates a new session whose SID and initial process-group ID
both equal the caller's PID. `EPERM` is exposed for the required leader
boundary.

`tests/session_hierarchy_probe.c` first records the fork child's inherited
session ID and process-group ID, which must match the parent's current
hierarchy. The child then calls `setsid`, verifies the returned SID equals its
PID and that `getsid(0)`, `getpgrp()`, and `getpgid(0)` all converge on that
PID. A second `setsid` and a `setpgid(0,0)` attempt both fail with `EPERM`,
locking the session-leader/process-group-leader invariant. The child reports
this state and blocks on a control pipe while the parent independently verifies
`getsid(child)` and `getpgid(child)` from the original session; the parent's own
SID/PGID remain unchanged. A nonexistent PID also locks the `ESRCH` query
boundary. The same executable runs through pinned tiny-c-compiler plus GNU ld
and mini-elf-toolchain.

## Next architectural promotion

Descriptor opening/I/O, pathname lifecycle, file sizing, and metadata now form
one executable filesystem baseline. `tests/metadata_probe.c` verifies the
native 144-byte stat ABI against real regular files and directories, correlates
pathname and descriptor metadata by device/inode, and proves both shrinking
and extending files through `ftruncate`, including zero-filled growth.
Bad descriptors, negative lengths, and missing paths exercise
`EBADF`/`EINVAL`/`ENOENT` boundaries. The same probe runs through pinned
tiny-c-compiler and both GNU ld and mini-elf-toolchain.

Descriptor I/O, filesystem namespace state, IPC/readiness, fork/wait,
`execve`, multithread-safe `posix_spawn`, targeted/group signaling,
process-group topology, and session creation/query now form one executable Unix
process hierarchy baseline. The next promotion should move beyond hierarchy
identifiers into a genuinely new control plane—such as controlling-terminal/
foreground-job semantics when they can be tested without host-dependent tty
assumptions, or a separately justified atfork architecture. More SID/PGID
aliases do not qualify as a new phase. Arbitrary post-fork libc use in a
multithreaded child remains outside the supported contract.
