# C and C.UTF-8 locale, multibyte, and wide-character runtime

mini-libc exposes the ISO C `"C"` locale plus a bounded `"C.UTF-8"` /
`"C.utf8"` `LC_CTYPE` mode. The text runtime shares one restartable
multibyte/wide-character conversion core across legacy conversion APIs,
wide-string helpers, oriented wide streams, and formatted I/O. C.UTF-8 supports
1-4 byte UTF-8 for valid Unicode scalar values without claiming locale
databases, collation, Unicode character properties, or stateful encodings.

## Public locale surface

`<locale.h>` exposes the standard locale categories used by the x86-64 Linux
target, `struct lconv`, `setlocale`, and `localeconv`.

The process starts in `"C"`. `LC_CTYPE` and `LC_ALL` additionally accept
`"C.UTF-8"` and `"C.utf8"`; querying those categories reports the selected
mode. The empty locale string selects mini-libc's implementation-native
`"C"` mode. Other categories retain the C-locale contract and reject named
non-C locales. Unsupported category values fail without changing the active
mode.

`localeconv()` remains process-lifetime C-locale data: decimal point `"."`,
empty grouping/currency/sign strings, and `CHAR_MAX` for unavailable monetary
placement/precision fields. The C.UTF-8 promotion changes character encoding,
not numeric or monetary conventions.

There is still no environment-variable locale selection, locale archive,
per-thread locale object, collation database, or mutable locale-specific
numeric/monetary data.

## C and C.UTF-8 multibyte model

`MB_CUR_MAX` is dynamic: 1 in `"C"` and 4 in `"C.UTF-8"`. In the C locale,
ASCII bytes `0x00..0x7f` are the complete valid multibyte character set and
higher standalone bytes report `EILSEQ`.

C.UTF-8 accepts canonical 1-4 byte UTF-8 sequences representing Unicode scalar
values through U+10FFFF. Invalid lead/continuation bytes, overlong forms,
surrogates, values above U+10FFFF, and incomplete sequences are rejected through
the restartable conversion contract. The public Linux x86-64 `EILSEQ` value is
84 and `strerror(EILSEQ)` returns `Invalid or incomplete multibyte or wide
character`.

The legacy `mblen`, `mbtowc`, `wctomb`, `mbstowcs`, and `wcstombs`
entry points delegate to the same restartable core. Reset/query operations stay
in the initial state because both supported encodings are stateless between
completed characters.

## Restartable wide-character surface

`<wchar.h>` exposes `mbstate_t`, `mbsinit`, `mbrtowc`, `wcrtomb`,
`mbsrtowcs`, `wcsrtombs`, and the basic wide-string helpers.

Caller-owned conversion state tracks incomplete UTF-8 sequences across
`mbrtowc` calls. Completed conversions reset to the initial state; a null
state pointer uses the implementation's restartable contract without requiring
callers to share an object. C-locale nonzero ASCII consumes one byte. C.UTF-8
may consume one through four bytes and returns `(size_t)-2` while a valid
sequence remains incomplete. Invalid input reports `(size_t)-1/EILSEQ`.
`wcrtomb` emits one C-locale byte or a canonical 1-4 byte UTF-8 sequence.

`mbsrtowcs` and `wcsrtombs` support sizing and bounded conversion while
preserving source-pointer progress. Illegal input leaves already converted
output intact and identifies the offending source position. Wide-string
comparison/copy/length helpers remain allocation-free and locale-independent.

## Oriented wide stream I/O

`<wchar.h>` exposes `fwide`, wide character/line/string I/O, and `ungetwc`.
An unoriented stream binds its wide encoding when it first becomes
wide-oriented. In `"C"` that encoding is single-byte ASCII; under
`"C.UTF-8"` it is UTF-8. Later process-locale changes do not mutate an already
wide-oriented stream's encoding.

Wide I/O continues through the shared buffered `FILE` state machine rather
than raw descriptor side paths. UTF-8 reads may consume multiple underlying
bytes per wide character; UTF-8 writes emit canonical multibyte sequences.
`ungetwc` pushes back the complete encoded sequence and logical positioning
accounts for all of its bytes. Invalid or incomplete stream data reports
`EILSEQ` and sets the stream error indicator.

Byte-oriented streams still reject wide I/O and wide-oriented streams reject
byte/block/byte-formatted I/O. Positioning and buffering preserve orientation;
successful `freopen` resets the rebound stream to unoriented state so it can
bind a fresh encoding.

## Wide formatted output baseline

Wide formatted output reuses the existing formatter parser, integer/floating
conversion engine, public `va_list` ABI, and FILE/memory sink contracts. It
does not maintain a second format grammar.

The formatter now carries explicit encoding and narrow-vs-wide-family semantics.
Narrow `printf`/`fprintf`/`snprintf` use the active `LC_CTYPE` encoding
for `%lc` and `%ls`. In C.UTF-8 those conversions emit canonical UTF-8;
narrow `%ls` precision remains a byte bound and never splits a multibyte
character.

Wide `fwprintf`/`vfwprintf` encode the wide format through the stream's
orientation-time encoding, render through the same formatter, then validate and
count the rendered multibyte text as wide characters before writing the exact
encoded bytes. This keeps a C.UTF-8-oriented stream UTF-8 even if the process
locale later returns to `"C"`.

`swprintf`/`vswprintf` use the current `LC_CTYPE` mode and decode the
shared formatter result back into `wchar_t`. Wide-family field widths and
string precisions count wide characters, while the corresponding narrow-family
`%ls` precision continues to count emitted bytes. Non-ASCII wide format
literals, `%lc`, `%ls`, and multibyte `%s` are therefore executable in
C.UTF-8. The C-locale behavior remains strict: unrepresentable wide values or
invalid multibyte strings report `EILSEQ`.

Wide-memory truncation retains the existing deterministic mini-libc contract:
a terminated prefix is stored when space permits, the call returns `EOF`, and
`ERANGE` is reported. Successful calls return the number of generated wide
characters, not the number of underlying UTF-8 bytes.

## Wide formatted input baseline

`<wchar.h>` now exposes `fwscanf`/`vfwscanf`, `wscanf`/`vwscanf`, and
`swscanf`/`vswscanf`. The implementation does not maintain a second scanner:
wide FILE and wide-string sources feed the existing scanner parser, integer and
floating conversion core, scanset machinery, matching/input-failure model, and
public `va_list` argument cursor.

Wide FILE scanning requires or establishes wide stream orientation and consumes
characters through the existing buffered wide read/unget helpers. Wide-memory
scanning advances a private `wchar_t` cursor and supports the same single
lookahead rollback invariant. The C-locale format is validated as ASCII before
dispatch; an unrepresentable format character reports `EILSEQ`.

For character-sequence conversions, the source and destination width are
independent. A byte source with `%ls`, `%lc`, or `%l[` converts through
`mbrtowc`; a wide source with ordinary `%s`, `%c`, or `%[` converts
through `wcrtomb`; an `l` destination from a wide source stores `wchar_t`
directly. Invalid C-locale data reports `EILSEQ`. Field width, suppression,
integer/floating conversions, scansets, and matching-versus-input-failure
semantics remain those of the shared scanner rather than wrapper-specific rules.

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
`swprintf`/`vswprintf`, truncation, C-locale rejection, C.UTF-8 non-ASCII wide
format literals, byte-bounded narrow `%ls`, wide-character-bounded `%s`/`%ls`,
stream-bound encoding across later locale changes, integer/floating formatter
reuse, genuine wide-argument FILE round trips, and wide formatted input through
`fwscanf`/`vfwscanf`, `wscanf`/`vwscanf`, and `swscanf`/`vswscanf`. The scan
coverage mixes integer, floating, narrow/wide string and character destinations,
scansets, FILE/stdin/wide-memory sources, orientation checks, and `EILSEQ` in the
same freestanding executable.

`tests/locale_differential.c` runs the host libc under `setlocale(LC_ALL, "C")`
and compares the directly comparable locale and legacy conversion behavior
against renamed mini-libc implementations. `tests/wchar_differential.c` does
the same for restartable conversions and wide strings; it explicitly resets
conversion state after error cases instead of depending on the standard's
unspecified post-error state.

The pinned tiny-c buffering integration directly executes `fputws` and `fgetws`
on an oriented buffered `tmpfile`, mixes the result with the existing
`fgetwc`/`ungetwc` coverage, and compiles and executes ordinary `swprintf` and
`fwprintf` together with caller-owned-`va_list` `vswprintf` and `vfwprintf`.
It also passes genuine `wchar_t *` and wide character arguments through
`%ls`/`%lc`, then executes wide formatted input over FILE and memory sources
through ordinary and public-`va_list` entry points. The same binary is linked
and executed through the pinned mini-elf-toolchain, so both wide formatted
output and input retain the three-repo executable gate while GCC/Clang
freestanding probes cover the public behavior.

## Phase boundary and next frontier

The C.UTF-8 conversion runtime and formatted-output integration are now one
executable baseline: restartable conversion, orientation-bound stream encoding,
narrow `%lc`/`%ls`, wide format literals, wide `%s`/`%lc`/`%ls`, FILE
output, bounded wide memory, ordinary variadics, and public `va_list` all reuse
the established formatter and buffered FILE machinery.

This remains a deliberately bounded internationalization model. There is no
locale database, collation, Unicode character-property/case mapping,
per-thread locale object, stateful multibyte encoding, or locale-sensitive
numeric/monetary formatting beyond the C conventions.

The next coherent text-runtime frontier is **C.UTF-8 formatted input
integration**. Wide input still uses the proven shared scanner, but its wide
format adapter retains the older ASCII-format boundary. A follow-on should make
non-ASCII wide literal directives and multibyte/wide string conversions obey
the selected or stream-bound encoding without creating a second scanner, and
must preserve matching-vs-input-failure and one-lookahead rollback semantics.
