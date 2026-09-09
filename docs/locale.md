# C locale, multibyte, and wide-character baseline

mini-libc currently implements one deliberate locale: the ISO C `"C"` locale.
The text runtime now includes locale selection/querying, a single-byte C-locale
multibyte model, restartable wide-character conversions, and a basic wide-string
core without claiming a locale database, UTF-8 locale, or stateful encoding.

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

Those entry points now delegate to the restartable wide-character core instead
of maintaining a second decoder/encoder. The C-locale ASCII/EILSEQ rules
therefore have one production source of truth.

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

`<wchar.h>` now exposes a concrete `mbstate_t` plus:

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

## Executable evidence

`tests/locale_probe.c` remains the freestanding baseline for locale selection,
`lconv`, legacy multibyte behavior, ASCII/NUL boundaries, `0x80` rejection,
sizing/truncation, errno preservation, and `EILSEQ` failures.

`tests/wchar_probe.c` is a second freestanding probe linked only against
mini-libc. It directly exercises caller-owned and null restartable state,
`mbrtowc` incomplete/reset/error boundaries, `wcrtomb`, bounded and sizing
restartable bulk conversion, source-pointer updates, error recovery, and the
wide-string core.

`tests/locale_differential.c` runs the host libc under `setlocale(LC_ALL, "C")`
and compares the directly comparable locale and legacy conversion behavior
against renamed mini-libc implementations. `tests/wchar_differential.c` does
the same for restartable conversions and wide strings; it explicitly resets
conversion state after error cases instead of depending on the standard's
unspecified post-error state.

The pinned tiny-c integration still compiles every production C source and its
legacy multibyte calls now execute through the restartable core. The same
integration is linked and executed through the pinned mini-elf-toolchain, so
this phase retains the three-repo executable gate while GCC/Clang freestanding
probes directly cover every newly public entry point.

## Phase boundary and next frontier

This remains a C-locale text runtime, not a Unicode or internationalization
claim. It does not implement UTF-8 decoding/encoding, stateful multibyte
encodings, locale databases, per-thread locales, collation, locale-aware ctype,
or non-C numeric/monetary formatting.

The strongest next text-runtime frontier is **wide stdio orientation and stream
I/O**: introduce stream orientation through `fwide`, then build coherent
`fgetwc`/`getwc`/`getwchar`, `fputwc`/`putwc`/`putwchar`, and `ungetwc` behavior
on the proven restartable conversion layer and the existing buffered/locked
`FILE` architecture. That work must define byte-vs-wide orientation transitions,
EOF/error propagation, buffering, positioning/rebinding interaction, and
multi-thread serialization before claiming wide stream support. Broader
wide-string/memory helpers can follow on the same `<wchar.h>` base; UTF-8 or
additional locale support remains a separate encoding/data milestone with its
own executable evidence.
