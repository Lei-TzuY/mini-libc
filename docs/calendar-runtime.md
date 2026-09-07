# Calendar runtime ABI and phase status

mini-libc now layers a bounded C11 calendar runtime on top of the existing
64-bit scalar time runtime and compiler-native static TLS substrate.

The public calendar surface is:

```c
struct tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

char *asctime(const struct tm *timeptr);
char *ctime(const time_t *timer);
struct tm *gmtime(const time_t *timer);
struct tm *localtime(const time_t *timer);
time_t mktime(struct tm *timeptr);
size_t strftime(char *restrict s, size_t maxsize,
                const char *restrict format,
                const struct tm *restrict timeptr);
```

## Calendar and timezone policy

Calendar conversion uses the proleptic Gregorian calendar and the Unix epoch.
Negative timestamps are supported, so `-1` converts to
`1969-12-31 23:59:59` and round-trips through `mktime` even though `(time_t)-1`
is also the standard failure sentinel.

This phase deliberately defines the process local timezone as **fixed UTC**:

- `gmtime` and `localtime` therefore expose the same broken-down fields;
- `tm_isdst` is always zero for runtime-generated values;
- `mktime` interprets its input as UTC and normalizes it back to UTC;
- `%z` is `+0000` and `%Z` is `UTC`.

There is no timezone database, `TZ` environment interpretation, daylight-saving
transition table, locale-selected timezone name, or leap-second table in this
phase. Those capabilities must not be inferred from the presence of
`localtime`, `mktime`, `%z`, or `%Z`.

## Conversion and normalization

`gmtime`/`localtime` derive year, month, day, weekday, day-of-year and clock
fields without calling the kernel. The scalar timestamp is divided into a
floor-based day number plus a non-negative second-of-day, so pre-1970 values do
not inherit C's truncating signed-division behavior.

The civil conversion handles Gregorian leap and century rules, including year
2000 as a leap year and year 2100 as a common year. A result whose `tm_year`
cannot be represented by the public `int` field is rejected with `ERANGE`.
Null timestamp pointers are rejected with `EINVAL`.

`mktime` accepts out-of-range month, day, hour, minute and second fields and
normalizes them through the same civil conversion. Month overflow first moves
across years, day overflow moves across civil days, and clock overflow moves
across day boundaries. The normalized `struct tm` replaces the caller's input
on success and `tm_isdst` becomes zero. Because the local policy is UTC,
`mktime(gmtime(&t))` is an epoch round-trip whenever the broken-down year is
representable. Successful conversion preserves the incoming `errno` value.

## Thread-local static results

C calendar APIs historically return pointers to static objects. mini-libc uses
real compiler-native `_Thread_local` storage instead of one process-global
object:

- `gmtime` and `localtime` share one `struct tm` slot per thread;
- `asctime` and `ctime` share one 26-byte text slot per thread.

A later call in the same thread may overwrite the corresponding slot, matching
the traditional static-result model. Calls from another C11 thread use distinct
addresses and do not overwrite the caller's result. This depends on the bounded
local-exec TLS ABI documented in `docs/compiler-tls.md`; it is not a synthetic
thread-indexed array.

`asctime` emits the conventional 26-byte C-locale form such as
`Tue Feb 29 00:00:00 2000\n`. This bounded implementation accepts four-digit
calendar years 0000 through 9999 and otherwise rejects invalid textual fields
with `EINVAL` rather than manufacturing an oversized static result.

## C-locale `strftime`

`strftime` uses a fixed C locale and supports the following conversion set:

- names: `%a`, `%A`, `%b`/`%h`, `%B`;
- date: `%C`, `%d`, `%D`, `%e`, `%F`, `%j`, `%m`, `%y`, `%Y`;
- clock: `%H`, `%I`, `%M`, `%S`, `%p`, `%r`, `%R`, `%T`;
- weekdays/weeks: `%u`, `%U`, `%V`, `%w`, `%W`, `%g`, `%G`;
- composites: `%c`, `%x`, `%X`;
- zone: `%z`, `%Z` under the fixed-UTC policy;
- control/literal: `%n`, `%t`, `%%`.

The C locale has no alternate digits or era, so `E`/`O` modifiers use the same
representation as their unmodified conversion. An unsupported/trailing
conversion returns zero with `EINVAL`. A destination that is too small returns
zero without changing `errno` and remains NUL-terminated when its size is
nonzero. Calendar text formatting is bounded to years 0000 through 9999.

## Executable evidence

The freestanding `time_probe` keeps all existing real-kernel scalar-time checks
and adds deterministic calendar evidence for:

- Unix epoch and pre-epoch conversion;
- the 2000 leap day and 2100 non-leap century boundary;
- the signed-32-bit 2038 timestamp boundary;
- `mktime` cross-year clock normalization and the valid epoch value `-1`;
- exact `asctime`/`ctime` bytes;
- C-locale names, day-of-year, Sunday/Monday week numbers, ISO week/year, UTC
  zone text, and composite `strftime` forms;
- small-buffer and unsupported-format behavior;
- out-of-range `time_t` rejection when `tm_year` cannot represent the result;
- distinct `gmtime`/`asctime` static-result addresses across a real C11 worker,
  while the main thread's TLS values remain unchanged.

The hosted `time_test` proves that calendar conversion, normalization and text
formatting perform zero raw `clock_gettime` calls while retaining the existing
fake-clock scalar-time coverage.

Pinned tiny-c compiles the production calendar runtime and a dedicated time
integration executable that performs `gmtime`, `mktime`, `strftime`,
`asctime`/`ctime`, and the same cross-thread TLS-isolation check. The executable
is linked and run through both GNU `ld` and the pinned mini-elf-toolchain, so the
calendar static results exercise the real local-exec TLS relocation/program-header
path rather than host libc storage.

## Phase boundary and promotion

This phase closes a UTC/C-locale calendar baseline by joining the previously
independent scalar-time and compiler-TLS substrates. It does not claim timezone
or locale infrastructure.

A higher calendar phase should therefore require a new capability such as a
real `TZ`/offset/DST policy with deterministic transition evidence or broader
locale-backed formatting. Repeating more UTC date vectors would be corner-case
farming. The next repository phase should still be chosen from all remaining
live standard-runtime gaps after a fresh architecture audit rather than assuming
timezone work is automatically the highest-value frontier.
