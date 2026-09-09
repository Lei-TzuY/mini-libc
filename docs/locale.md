# C locale, multibyte, and wide-character baseline

mini-libc currently implements one deliberate locale: the ISO C `"C"` locale.
The text runtime includes locale selection/querying, a single-byte C-locale
multibyte model, restartable wide-character conversions, a basic wide-string
core, oriented wide stream character/line I/O, and a bounded wide formatted
output baseline without claiming a locale database, UTF-8 locale, or stateful
encoding.

## Public locale surface

`<locale.h>` exposes the standard locale categories used by the x86-64 Linux
target, `struct lconv`, `setlocale`, and `localeconv`.

The process starts in the C locale and this phase never leaves it:

- `setlocale(category, NULL)` queries the current locale and returns `"C"` for
  every supported category.
- `setlocale(category, "C")` succeeds and returns `"C"`.
- `setlocale(category, "")` selects mini-libc's implementation-native locale,
  which is deliberately the same C locale in this phase.
- unsupported category values and named locales other than `"C"` fail by
  returning null without changing the existing locale.

`localeconv()` returns process-lifetime static C-locale data. The decimal point
is `"."`; grouping, thousands separators, currency strings, and sign strings
are empty; unavailable monetary placement and precision fields use `CHAR_MAX`
for the target, currently 127.

There is no environment-variable locale selection, locale archive, per-thread
locale, or mutable locale state yet.

## Single-byte multibyte model

`MB_CUR_MAX` is 1. In the C locale mini-libc treats ASCII bytes `0x00` through
`0x7f` as the complete valid multibyte character set. Bytes `0x80` through
`0xff` are not accepted as standalone multibyte characters and report `EILSEQ`.
The public Linux x86-64 errno value for `EILSEQ` is 84, and `strerror(EILSEQ)`
returns the fixed C-locale-style message `Invalid or incomplete multibyte or
wide character`.

The legacy `<stdlib.h>` surface remains available:

- `mblen`
- `mbtowc`
- `wctomb`
- `mbstowcs`
- `wcstombs`

Those entry points delegate to the restartable wide-character core instead of
maintaining a second decoder/encoder. The C-locale ASCII/EILSEQ rules therefore
have one production source of truth.

The conversion is stateless. `mblen(NULL, ...)`, `mbtowc(..., NULL, ...)`, and
`wctomb(NULL, ...)` therefore report an initial-state result of zero. For a
non-null input with a zero byte limit, the legacy byte-reading operations return
`-1` without fabricating `EILSEQ` because no input byte was examined.

`wchar_t` is the existing signed 32-bit mini-libc target type. Valid C-locale
characters map directly between their unsigned ASCII byte value and the same
`wchar_t` value. A wide value outside `0..0x7f` is not representable in the
current multibyte encoding and causes the applicable conversion to report
`EILSEQ`.

## Restartable wide-character surface

`<wchar.h>` exposes a concrete `mbstate_t` plus:

- `mbsinit`
- `mbrtowc`
- `wcrtomb`
- `mbsrtowcs`
- `wcsrtombs`
- `wcslen`
- `wcscmp`
- `wcscpy`

The current C-locale encoding is stateless, so every valid conversion begins and
ends in the initial state. Caller-owned `mbstate_t` objects are normalized back
to zero state after each conversion. A null state pointer does not require or
mutate process-global conversion storage, which keeps independent callers free
of an unnecessary shared-state race.

`mbrtowc` returns `1` for a nonzero ASCII byte, `0` for the null character,
`(size_t)-2` when the byte limit is zero, and `(size_t)-1` plus `EILSEQ` for a
byte above `0x7f`. A null source performs the standard reset/query operation and
does not modify the output wide-character object. `wcrtomb(NULL, ..., ps)` is a
reset/query and returns the one-byte length of the C-locale null character.

`mbsrtowcs` and `wcsrtombs` support bounded destination conversion and null
sizing destinations. In sizing mode the source pointer is not modified. In
bounded mode reaching the terminator sets `*src` to null; exhausting the output
bound leaves `*src` at the first unconverted element. An illegal source returns
`(size_t)-1`, leaves already converted output intact, and points `*src` at the
offending byte or wide character.

The basic wide-string functions are allocation-free and independent of locale
state. `wcscmp` guarantees the usual negative/zero/positive ordering contract
rather than a specific magnitude.

## Oriented wide stream I/O

`<wchar.h>` exposes the C-locale wide stream baseline:

- `fwide`
- `fgetwc`, `getwc`, `getwchar`
- `fputwc`, `putwc`, `putwchar`
- `fgetws`, `fputws`
- `ungetwc`
- `fwprintf`, `wprintf`, `swprintf`
- `vfwprintf`, `vwprintf`, `vswprintf`

An unoriented stream becomes wide-oriented on its first successful wide I/O
operation or through `fwide(stream, positive_mode)`. Byte-oriented streams reject
wide operations with `EINVAL` and a sticky stream error; wide-oriented streams
likewise reject byte, block, and byte-formatted I/O. Positioning and buffering
preserve orientation. A successful `freopen` rebind resets the stream to the
unoriented state so the rebound stream can establish a fresh orientation.

Wide character and wide line/string APIs share the same private conversion and
buffered `FILE` path rather than issuing parallel raw-descriptor I/O. In the
current single-byte C locale each successful wide character maps through
`mbrtowc`/`wcrtomb` to exactly one ASCII byte. Invalid input bytes or
unrepresentable wide values report `EILSEQ` and set the stream error indicator.

`fgetws` validates orientation, readability, and the update-stream write-to-read
synchronization barrier before any transfer, including the `n == 1` boundary.
It stores at most `n - 1` wide characters, retains an encountered newline, and
always terminates a successful result with a wide null. EOF after at least one
character returns the partial line; immediate EOF returns null. `n == 1`
returns an empty wide string without advancing the logical file position.

`fputws` validates wide orientation and writability even for an empty source,
then emits source characters through the same wide write core as `fputwc`; the
terminating wide null is not written. Non-empty output continues to inherit the
existing buffered/update-stream write contract from `FILE`.

`ungetwc` uses the existing guaranteed one-byte pushback slot. A successful
pushback clears EOF and updates the logical position; a second pending pushback
or an incompatible update-stream state is rejected deterministically.

## Wide formatted output baseline

Wide formatted output deliberately reuses the existing narrow formatter parser,
integer/floating conversion engine, public `va_list` ABI, and FILE/memory sink
contracts instead of maintaining a second format grammar. A wide format string
is first validated and mapped through the C-locale ASCII encoding, rendered by
the proven formatter, then converted back through the wide-character layer for
a wide-oriented FILE or `wchar_t` memory destination.

`fwprintf`/`vfwprintf` require a writable wide-oriented stream. An unoriented
stream becomes wide-oriented; a byte-oriented stream is rejected with `EINVAL`
and a sticky stream error. Successful bytes are emitted through the same
buffered `FILE` core as `fputwc` after C-locale conversion, so positioning,
line/full buffering, flush, update-stream synchronization, and close/exit
lifecycle remain shared.

`wprintf`/`vwprintf` are the stdout counterparts. `swprintf`/`vswprintf` render
to a bounded wide destination. On success they return the number of generated
wide characters excluding the terminator and preserve the caller's `errno`.
When the result does not fit, this baseline stores a deterministic terminated
prefix when space permits, returns `EOF`, and reports `ERANGE`; a zero-sized
destination likewise returns `EOF`/`ERANGE` without dereferencing a null buffer.

The baseline inherits the already executable narrow formatter conversions for
integer, floating, narrow `%s`, narrow `%c`, `%%`, flags, field width,
precision, ordinary variadics, and public `va_list`. In the C locale a narrow
`%s` or `%c` result containing a byte above `0x7f` is not representable as a
wide output character and reports `EILSEQ`. A non-ASCII wide format character is
rejected for the same reason.

This first wide-format slice does **not** yet claim `%ls`/`%lc` conversion
parity. Those length-modified wide character/string conversions remain a
separate formatter-integration frontier rather than being emulated by casting a
`wchar_t *` into the narrow `%s` path. Wide formatted input is also still
separate.

## Executable evidence

`tests/locale_probe.c` remains the freestanding baseline for locale selection,
`lconv`, legacy multibyte behavior, ASCII/NUL boundaries, `0x80` rejection,
sizing/truncation, errno preservation, and `EILSEQ` failures.

`tests/wchar_probe.c` is a second freestanding probe linked only against
mini-libc. It directly exercises caller-owned and null restartable state,
`mbrtowc` incomplete/reset/error boundaries, `wcrtomb`, bounded and sizing
restartable bulk conversion, source-pointer updates, error recovery, and the
wide-string core.

`tests/wide_stdio_probe.c` exercises stream orientation, byte-vs-wide conflicts,
wide character and line/string I/O, bounded/newline `fgetws`, `ungetwc`, EOF and
`EILSEQ` propagation, positioning, `freopen` orientation reset, stdin/stdout
wide paths, ordinary and `v*` wide formatted FILE/stdout output, bounded
`swprintf`/`vswprintf`, truncation, invalid C-locale format/data, and integer and
floating formatter reuse in a freestanding executable.

`tests/locale_differential.c` runs the host libc under `setlocale(LC_ALL, "C")`
and compares the directly comparable locale and legacy conversion behavior
against renamed mini-libc implementations. `tests/wchar_differential.c` does
the same for restartable conversions and wide strings; it explicitly resets
conversion state after error cases instead of depending on the standard's
unspecified post-error state.

The pinned tiny-c buffering integration directly executes `fputws` and `fgetws`
on an oriented buffered `tmpfile`, mixes the result with the existing
`fgetwc`/`ungetwc` coverage, and now compiles and executes ordinary `swprintf`
and `fwprintf` together with caller-owned-`va_list` `vswprintf` and `vfwprintf`.
The same binary is linked and executed through the pinned mini-elf-toolchain, so
wide formatted output retains the three-repo executable gate while GCC/Clang
freestanding probes cover all six newly public entry points.

## Phase boundary and next frontier

This remains a C-locale text runtime, not a Unicode or internationalization
claim. It does not implement UTF-8 decoding/encoding, stateful multibyte
encodings, locale databases, per-thread locales, collation, locale-aware ctype,
or non-C numeric/monetary formatting.

Wide formatted output transport is now executable across FILE, stdout, bounded
memory, ordinary variadics, and public `va_list` while sharing the existing
formatter engine. The strongest next text-runtime work should close **wide
conversion parity and input formatting**, not farm more numeric format vectors.
A coherent next slice should first make `%lc`/`%ls` consume genuine wide
arguments through the shared formatter architecture, then use the same parser /
conversion discipline to introduce `fwscanf`/`vfwscanf`/`swscanf` without
building a second scanner. Any such phase must preserve orientation, buffering,
return-count/error behavior, and pinned GCC/Clang/tiny-c/mini-elf execution.
UTF-8 or broader locale data remains a separate encoding milestone.
