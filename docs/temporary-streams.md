# Anonymous temporary stream ABI and phase status

mini-libc now exposes `tmpfile()` as a real anonymous owned `FILE` rather than a pathname convention or host-libc fallback.

## Backing descriptor

`tmpfile()` allocates normal owned `FILE` storage and opens an unnamed Linux temporary inode through the existing raw `openat` boundary:

```text
openat(AT_FDCWD, "/tmp", O_RDWR | O_TMPFILE, 0600)
```

`O_TMPFILE` creates no directory entry, so there is no generated filename, unlink race, or namespace cleanup step. The resulting descriptor is seekable and read/write. Closing the last descriptor, or process termination, lets the kernel reclaim the backing inode automatically.

The implementation deliberately reuses the existing `openat` syscall surface instead of adding a second anonymous-file syscall. If the target filesystem or kernel rejects `O_TMPFILE`, `tmpfile()` returns null and exposes the positive kernel error through `errno`.

## FILE ownership and initialization

`fopen()` and `tmpfile()` share one private owned-stream initializer. A successful temporary stream therefore enters exactly the same live-stream registry and begins with the same lazy default buffering state as a pathname-backed owned stream.

The public contract is:

```c
FILE *tmpfile(void);
```

A temporary stream is readable, writable, seekable, and owned. It supports the existing byte/block/line/formatted I/O, logical positioning, update-stream barriers, `setvbuf`/`setbuf` policies, EOF/error indicators, and public variadic formatted I/O without a temporary-stream-specific path.

Allocation happens before descriptor creation. If FILE allocation fails, no raw open is issued. If anonymous descriptor creation fails, the allocated FILE object is released and the kernel error is propagated. Successful creation preserves the incoming `errno` value under the same successful-operation convention as the existing stream lifecycle.

## Flush, close, and process termination

`fclose()` uses the normal owned-stream sequence:

1. flush pending output;
2. close the anonymous descriptor;
3. unregister the stream;
4. release any libc-owned configured buffer;
5. release owned FILE storage.

Flush and close failures keep the established first-error precedence while still performing ownership cleanup.

A live temporary stream also participates in `__mini_stdio_flush_all()`, the exact sweep invoked by normal `exit()` after `atexit` handlers. Therefore pending temporary-stream output is synchronized by the same normal-termination path as every other writable FILE. `_Exit` continues to bypass stdio synchronization. At final process teardown, Linux closes remaining descriptors and destroys the unnamed backing object automatically.

## Executable evidence

The real file-stream probe creates an actual anonymous stream on the CI kernel and drives it through caller-provided full buffering, `fprintf`, `ftell`, `rewind`, `fscanf`, `SEEK_END`, write-after-read positioning, EOF, and `fclose`. No pathname is passed to the public API and the historical pathname-backed probe keeps its existing final-content contract.

The deterministic hosted buffering harness verifies the exact anonymous-open request, allocation-before-open ordering, allocation failure with zero open calls, open failure with FILE storage cleanup, registration of a successful temporary stream, pending-output publication through `__mini_stdio_flush_all()`, descriptor close, and FILE release.

The pinned tiny-c buffering integration separately compiles `tmpfile()` use, configures a caller-owned buffer, performs formatted output and seek/read verification, and closes the stream. The same executable is linked and run through GNU `ld` and the pinned mini-elf-toolchain, preserving the host-libc-independence gate.

## Phase boundary and next frontier

Anonymous temporary stream lifecycle is part of the executable FILE baseline. Stream rebinding through `freopen()` now reuses the same ownership, buffering, registry, synchronization, and close machinery for pathname redirection; see [`stream-rebinding.md`](stream-rebinding.md) for that contract.

This phase intentionally does not add `tmpnam`, expose a generated pathname, or create a separate temporary-file registry. Named temporary-file generation, wide-character/locale behavior, threading/TLS, and long-double I/O remain separate later phases.
