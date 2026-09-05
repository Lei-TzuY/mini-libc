# Stream buffering ABI and phase status

mini-libc now exposes configurable `FILE` buffering as an executable stream
policy rather than a fixed implementation detail. The public surface is:

```c
#define _IOFBF 0
#define _IOLBF 1
#define _IONBF 2
#define BUFSIZ 256

int setvbuf(FILE *restrict stream, char *restrict buf, int mode, size_t size);
void setbuf(FILE *restrict stream, char *restrict buf);
```

The default inherited streams retain the established behavior: `stdin` and
`stdout` use the built-in 256-byte storage lazily, while `stderr` is unbuffered.
Owned streams created by `fopen` begin with the same default lazy inline storage.

## Storage and ownership model

Each live `FILE` retains two private 256-byte inline arrays as dependency-free
default storage, but the active read/write storage is selected through private
buffer pointers plus a capacity. This keeps basic inherited stdio independent of
the allocator while allowing a configured stream to replace its default storage.

For `_IOFBF` and `_IOLBF`:

- a non-null `buf` is caller-owned and is never freed by mini-libc;
- a null `buf` asks mini-libc to allocate `size` bytes and the stream records
  ownership of that allocation;
- replacing or closing a libc-owned buffer releases it exactly once;
- switching to a new configuration is transactional: a newly allocated candidate
  is released if synchronization fails and the old stream configuration remains
  live.

`_IONBF` uses no active read/write staging buffer. `setbuf(stream, buf)` is the
`BUFSIZ` full-buffer convenience form; `setbuf(stream, NULL)` selects unbuffered
mode.

Buffered modes require a nonzero size. Invalid streams, modes, or zero-sized
buffered requests fail deterministically with `EOF`/`EINVAL`. Successful
`setvbuf` preserves the incoming `errno` value.

## Full buffering

Full buffering preserves the established mini-libc timing contract. Filling a
buffer exactly to capacity does not by itself force a raw write. The bytes remain
pending until one of these events occurs:

- another output byte needs space;
- `fflush` is requested;
- a positioning operation synchronizes output;
- the buffering mode changes;
- `fclose` runs;
- normal `exit` performs the live-stream flush sweep.

This preserves the pre-existing deterministic 256-byte full-buffer behavior while
making the capacity configurable.

## Line buffering

Line-buffered output uses the same pending-output state and partial-write retry
logic as full buffering. The additional rule is executable: the first newline in
the current input chunk is accepted into the buffer and immediately flushes all
pending bytes through the normal raw-write path. Processing then continues after
that newline, so multiple lines in one high-level write retain the same ordering.

A line-buffered stream with no newline remains buffered until another normal
flush boundary occurs.

## Unbuffered mode

Unbuffered writes bypass pending output and drive the existing short-write/error
loop directly. Unbuffered reads request caller-visible bytes directly from the raw
read boundary rather than prefetching an internal block. The existing sticky
EOF/error indicators and update-stream direction barriers remain in force.

A mode change after input is not allowed to silently discard prefetched bytes.
`setvbuf` reuses the existing positioning layer with `fseek(stream, 0, SEEK_CUR)`.
That operation compensates for unread cache and one-byte pushback before the old
buffer can be released. If the underlying seek fails, the mode change fails and
the old cache, buffer, logical cursor, and configuration remain usable.

## Positioning and update streams

Buffer configuration does not create a second cursor model. `ftell`, `fseek`,
`rewind`, `fread`, `fwrite`, `fgetc`, `fgets`, `ungetc`, formatted input, and
formatted output continue to use the same `FILE` state machine.

In particular:

- pending output is flushed before a successful mode transition;
- unread input and pushback are reconciled through the shared positioning logic;
- write-to-read and read-to-write synchronization barriers are unchanged;
- a failed synchronization step leaves the old buffering state intact;
- configured buffer capacity affects refill/write batching, not logical stream
  positions.

## Close and normal-exit lifecycle

`fclose` flushes pending output, closes the descriptor, removes the stream from the
live-stream registry, releases any libc-owned configured buffer, and then releases
owned `FILE` storage. Buffer release still occurs when flush or close reports an
error, so an error path cannot leak a libc-owned buffer.

Normal `exit` continues to run `atexit` handlers before sweeping all live writable
streams with `fflush`. Configured full and line buffers therefore participate in
the same exit contract as the original inline buffer. `_Exit` continues to bypass
stdio flushing entirely.

## Executable evidence

The real owned-file probe observes buffering from a second independently opened
stream. It proves that a caller-provided full buffer remains invisible at exact
capacity, the next byte causes the full block to become visible, explicit flush
publishes the remainder, line buffering publishes a newline-terminated record,
`setbuf` uses caller storage without transferring ownership, libc-owned storage
can be installed and replaced, and unbuffered output becomes immediately visible.
The probe restores the historical final `ABC` file content so earlier pathname
lifecycle evidence stays intact.

A deterministic hosted harness measures raw read/write/seek calls. It proves
configurable refill sizes, deferred full-buffer writes, newline-driven line flush,
unbuffered direct output, libc-owned allocation/free, caller-buffer non-ownership,
input-cache realignment through `SEEK_CUR`, and preservation of unread cached data
when the realignment seek fails.

The pinned tiny-c integration separately compiles a buffering executable using
`setvbuf` and `setbuf`. The same executable is linked and run through GNU `ld` and
the pinned mini-elf-toolchain, checks visibility through independent owned-file
readers, exercises caller-owned/full/line/unbuffered/libc-owned modes, and passes
host-libc-independence inspection.

## Phase boundary and next frontier

Configurable buffering and buffer ownership are now part of the executable FILE
baseline. This phase does not add an alternate stream implementation: it extends
the existing live-stream registry, read/write barriers, positioning layer,
flush/close lifecycle, and normal-exit sweep.

The next higher architectural frontier is **anonymous temporary stream lifecycle**.
A coherent next slice should add a real `tmpfile`-style owned stream backed by an
anonymous Linux descriptor, integrate it with the same FILE registry and buffering
policies, prove automatic descriptor/storage reclamation on `fclose` and normal
process exit, and keep temporary-stream creation independent from host libc. That
crosses the raw-syscall, descriptor, FILE-ownership, buffering, and termination
layers in one executable capability.

`%p`/`%n`, C11 exclusive-create modes, wide-character I/O, locale-sensitive
behavior, threading/TLS, long-double I/O, and allocator tuning remain separate
later phases.
