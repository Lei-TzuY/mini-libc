# Stream rebinding ABI and phase status

mini-libc now exposes `freopen()` as an in-place `FILE` lifecycle transition rather than as a close/open convenience wrapper.

```c
FILE *freopen(const char *restrict filename, const char *restrict mode,
              FILE *restrict stream);
```

## Successful rebinding

The current implementation requires a non-null pathname, a valid live stream, and a mode accepted by the same pathname-mode parser used by `fopen`. mini-libc does not currently implement the implementation-defined `freopen(NULL, mode, stream)` form.

A successful rebind performs this ordered transition:

1. validate the pathname and parse the new mode before mutating the old stream;
2. flush pending output from the old association;
3. close the old descriptor;
4. open the replacement pathname through the existing raw `openat` boundary;
5. install the new descriptor and access/append mode into the same `FILE` object;
6. clear EOF/error, update-direction, pending-output, unread-input, and pushback state.

The returned pointer is the original `FILE *`. The live-stream registry link is not removed/reinserted on success, so registry identity remains stable.

## Ownership and configured buffering

Rebinding preserves the ownership of the `FILE` object itself. A heap-owned stream created by `fopen` remains owned; an inherited static stream such as `stdout` or `stderr` remains non-owned. The parsed pathname mode therefore never turns a predefined static stream into heap-owned storage.

The current configured buffering policy and storage also survive a successful rebind:

- `_IOFBF`, `_IOLBF`, and `_IONBF` policy bits are preserved;
- a caller-provided buffer remains caller-owned and active;
- a libc-owned configured buffer remains owned by the same `FILE` and is not reallocated;
- lazy/default inline storage remains available through the existing stdio core.

Only the descriptor association and logical stream state are replaced. This lets a caller configure a stream once and redirect it without losing buffer ownership or capacity.

## Failure state

Invalid arguments or an invalid mode are detected before flushing or closing the old stream. These deterministic mini-libc validation failures return null, set `EINVAL`, and leave the existing stream usable.

Once rebinding has begun, failure closes the old association rather than pretending that the original stream is still usable:

- a flush failure is remembered as the first error, the old descriptor is still closed, and no replacement open is attempted;
- a close failure prevents the replacement open;
- a replacement `openat` failure is returned after the old descriptor has been closed.

Owned `FILE` objects on those post-transition failures are unregistered, any libc-owned configured buffer is released, and the owned object is freed. The old pointer must not be reused. Inherited static streams cannot be freed; they remain allocated but become invalid (`fd == -1`, no readable/writable mode). The first flush error takes precedence over a later close error.

Successful rebinding preserves the caller's incoming `errno`; failures publish the positive kernel or validation error according to the existing mini-libc convention.

## Executable evidence

The real kernel file-stream probe configures a caller-owned full buffer, leaves output pending, and then rebinds the same owned `FILE *`. It proves that pending bytes become visible in the old pathname before close, the new pathname starts with its own contents, the caller buffer remains active after rebinding, the same pointer can be written/rewound/read, and invalid modes leave the old association intact. It also exercises replacement-open failure and rebinding of inherited `stderr` without transferring heap ownership.

A deterministic hosted regression records the raw syscall order. A successful writable rebind must observe `write -> close -> open`; open failure observes `close -> open`; flush failure observes `write -> close` with no open; close failure observes only `close` with no replacement open. The same harness checks exact FILE/allocation cleanup and preservation of caller buffer storage.

The pinned tiny-c integration compiles an owned-stream rebind, checks pointer identity, preserves a caller buffer, validates old/new pathname contents, and verifies invalid-mode preservation. The same executable is linked and run through GNU `ld` and the pinned mini-elf-toolchain and remains subject to host-libc-independence inspection.

## Phase boundary

Stream rebinding now closes the pathname-backed FILE lifecycle that includes open/create/append/exclusive-create, configurable buffering, anonymous temporary streams, pathname rename/remove, positioning, close, and normal process termination.

The remaining small stdio names (`fgetpos`/`fsetpos`, `perror`, `tmpnam`) do not by themselves justify another architectural phase. The next promotion should therefore re-audit runtime-wide state ownership and reentrancy rather than farm isolated stdio wrappers. In particular, `errno`, the environment view, the stdio registry, and tokenizer state are still process-global. A TLS/reentrant-state phase is only valid if the pinned tiny-c compiler and mini-elf linker can provide executable TLS evidence; otherwise the project should choose the next independent subsystem whose required ABI is already supported.

Wide-character/locale behavior, long-double I/O, threading/TLS, named temporary-file generation, and broader filesystem metadata remain separate later surfaces.
