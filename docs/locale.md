# C locale and multibyte baseline

mini-libc currently implements one deliberate locale: the ISO C `"C"` locale.
This phase establishes the locale and multibyte contracts needed by later
wide-character work without claiming a locale database, UTF-8 locale, or
stateful encoding support.

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

The current `<stdlib.h>` surface includes:

- `mblen`
- `mbtowc`
- `wctomb`
- `mbstowcs`
- `wcstombs`

The conversion is stateless. `mblen(NULL, ...)`, `mbtowc(..., NULL, ...)`, and
`wctomb(NULL, ...)` therefore report an initial-state result of zero. For a
non-null input with a zero byte limit, the byte-reading operations return `-1`
without fabricating `EILSEQ` because no input byte was examined.

`wchar_t` is the existing signed 32-bit mini-libc target type. Valid C-locale
characters map directly between their unsigned ASCII byte value and the same
`wchar_t` value. A wide value outside `0..0x7f` is not representable in the
current multibyte encoding and causes `wctomb`/`wcstombs` to report `EILSEQ`.

`mbstowcs` and `wcstombs` support both bounded destination conversion and null
destination sizing. A bounded conversion that reaches its element limit before
the source terminator returns the converted count without adding a terminator.
An illegal input reports `(size_t)-1`; bytes/elements converted before the
illegal character remain in the destination, matching the controlled C-locale
host behavior used by the test suite.

## Executable evidence

`tests/locale_probe.c` is a freestanding probe linked only against mini-libc. It
checks category/query behavior, the complete C-locale `lconv` contract,
`MB_CUR_MAX`, reset calls, ASCII/NUL boundaries, `0x80` rejection, bulk sizing,
truncation, termination, errno preservation, and `EILSEQ` failures.

`tests/locale_differential.c` runs the host libc under `setlocale(LC_ALL, "C")`
and compares the directly comparable locale and single-byte conversion behavior
against renamed mini-libc implementations.

The pinned tiny-c integration compiles every production C source and directly
executes `setlocale`, `localeconv`, `MB_CUR_MAX`, `mbstowcs`, `wcstombs`, and an
invalid `wctomb` case. The same integration binary is linked and executed again
through the pinned mini-elf-toolchain, so this phase retains the three-repo
executable gate.

## Phase boundary and next frontier

This phase is intentionally not a Unicode or internationalization claim. It does
not implement `wchar.h`, `mbstate_t`, restartable conversions, wide-string
algorithms, wide stdio, UTF-8 decoding/encoding, collation, locale-aware ctype,
or non-C numeric/monetary formatting.

The strongest next text-runtime frontier is a real wide-character/restartable
conversion layer: public `<wchar.h>` and `mbstate_t`, `mbrtowc`, `wcrtomb`,
`mbsrtowcs`, `wcsrtombs`, plus a coherent basic wide-string core such as
`wcslen`, `wcscmp`, and `wcscpy`. That work should build on this single-byte C
locale state model first; UTF-8 or additional locale claims require separate
encoding and locale-data implementations with their own executable evidence.
