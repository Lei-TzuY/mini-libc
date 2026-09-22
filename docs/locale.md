# C and C.UTF-8 locale, multibyte, and wide-character runtime

mini-libc exposes the ISO C `"C"` locale plus a bounded `"C.UTF-8"` /
`"C.utf8"` `LC_CTYPE` mode. The text runtime shares one restartable
multibyte/wide-character conversion core across legacy conversion APIs,
wide-string helpers, oriented wide streams, formatted I/O, and wide-character
classification. C.UTF-8 supports 1-4 byte UTF-8 for valid Unicode scalar values
plus a pinned Unicode 15.1 classification/simple-case layer, without claiming a
locale database, collation, locale-tailored full case mappings, normalization,
or stateful encodings.

## Public locale surface

`<locale.h>` exposes the standard locale categories used by the x86-64 Linux
target, `struct lconv`, `setlocale`, and `localeconv`.

The process starts in `"C"`. `LC_CTYPE` and `LC_ALL` additionally accept
`"C.UTF-8"` and `"C.utf8"`. Each standard category is owned explicitly by
the process-locale state even though the bounded runtime currently permits only
`LC_CTYPE` to differ from `"C"`. Querying `LC_ALL` therefore returns
`"C"` only when every category is C; a mixed CTYPE=C.UTF-8 state returns an
opaque composite string that records C for the remaining categories. A copied
composite string can be passed back to `setlocale(LC_ALL, ...)` to restore the
same state.

An empty locale string resolves environment-driven selection in the order
`LC_ALL`, the category-specific variable such as `LC_CTYPE`, then `LANG`,
falling back to `"C"` when each candidate is absent or empty. For `LC_ALL`,
there is no additional category-specific variable, so the bounded model uses
`LC_ALL` then `LANG`. Unsupported environment values fail transactionally
without changing the active mode. Other categories retain the C-locale contract
and therefore reject a resolved non-C locale.

`localeconv()` remains process-lifetime C-locale data: decimal point `"."`,
empty grouping/currency/sign strings, and `CHAR_MAX` for unavailable monetary
placement/precision fields. The C.UTF-8 promotion changes character encoding,
not numeric or monetary conventions.

There is still no locale archive, per-thread locale object, collation database,
or mutable locale-specific numeric/monetary data. Environment lookup is
allocation-free and reuses the startup-backed `getenv` state; it does not
implicitly apply a locale until `setlocale(category, "")` is called.

## Unicode 15.1 wide classification and simple case mapping

`<wctype.h>` exposes the ISO C wide classification, descriptor, and case
transformation surface. In the `"C"` locale it deliberately retains the
ASCII-only contract. Under `"C.UTF-8"`, the same APIs switch to a generated
Unicode Character Database 15.1.0 substrate pinned to
`unicode-org/unicodetools@1882e4cca24a298184d685e6a3820428749d050d`.

The generated table uses `UnicodeData.txt` general categories plus
`PropList.txt` `White_Space`. mini-libc defines C.UTF-8 properties as:
letters plus `Nl` for `alpha`, `Nd` for `digit`, `Ll`/`Lu` for
lower/upper, `Cc` for control, `P*` for punctuation, Unicode
`White_Space` for space, HT plus `Zs` for blank, `L|M|N|P|S` for graph,
and graph plus `Zs` for print. `xdigit` intentionally remains the portable
ASCII hexadecimal set. `towlower`/`towupper` use UnicodeData simple
one-code-point mappings; locale/context-sensitive multi-code-point special
casing is outside this bounded contract.

The tables are compressed into property ranges plus simple mapping pairs and are
queried by binary search. Invalid scalar values and `WEOF` never acquire a
Unicode property and case transforms leave them unchanged. Locale switching is
observable: a non-ASCII code point classified in C.UTF-8 immediately returns to
the ASCII-only behavior after switching `LC_CTYPE` back to `"C"`.

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
lookahead rollback invariant. A wide format is encoded before dispatch using the
FILE stream's orientation-time encoding or, for wide-memory scanning, the current
`LC_CTYPE` mode. The shared scanner decodes non-ASCII literal directives back
to code points before matching wide input. This keeps a stream bound to C.UTF-8
even if the process locale later returns to `"C"`; an unrepresentable C-locale
format character still reports `EILSEQ`.

For character-sequence conversions, input field width and destination storage
width are independent. A byte source with `%ls` or `%lc` consumes complete
multibyte characters through the mode-aware restartable decoder, so one field
character may require one through four UTF-8 bytes. A wide source with ordinary
`%s` or `%c` emits each source code point through the selected stream or
memory encoding and advances the destination by the actual encoded byte count.
An `l` destination from a wide source stores `wchar_t` directly. Suppressed
wide-to-narrow conversions consume the input item without inventing a destination
encoding failure, while suppressed byte-to-wide conversions still decode enough
bytes to preserve multibyte character boundaries. Invalid encoded input reports
`EILSEQ`. Integer/floating conversions and matching-versus-input-failure
semantics remain those of the shared scanner rather than wrapper-specific rules.

Wide scansets are now codepoint-aware. Narrow scansets retain byte semantics,
while a wide FILE/string source decodes C/C.UTF-8 scanset members and ranges from
the wide format and compares decoded code points. Non-ASCII member/range,
negation, suppression, rollback, and stream-bound encoding behavior therefore
reuse the same scanner rather than falling back to encoded-byte membership.

## Executable evidence

`tests/locale_probe.c` remains the freestanding baseline for locale selection,
including `LC_ALL`/category/`LANG` precedence, empty-variable fallback,
unsupported-environment rollback, `lconv`, legacy multibyte behavior, ASCII/NUL
boundaries, `0x80` rejection, sizing/truncation, errno preservation, and
`EILSEQ` failures.

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
`%ls`/`%lc`, then executes C.UTF-8 wide formatted input over wide-memory,
narrow-memory, and orientation-bound FILE sources. The integration covers a
non-ASCII wide literal, wide-to-narrow UTF-8 `%s`, narrow-to-wide `%ls`/`%lc`,
and caller-owned-`va_list` `vfwscanf`/`vswscanf`. The same binary is linked and
executed through the pinned mini-elf-toolchain, so both wide formatted output and
input retain the three-repo executable gate while GCC/Clang freestanding probes
cover the public behavior.

## Phase boundary and next frontier

The C/C.UTF-8 text runtime now closes the basic encoding, formatted-I/O,
codepoint-wide scanset, and wide classification/case baseline. Restartable UTF-8
conversion, stream-bound orientation encoding, narrow/wide formatted
input/output, Unicode-scalar scansets, and Unicode 15.1 `wctype` behavior all
execute through shared runtime state rather than wrapper-local special cases.

This is still intentionally not a general locale database. There is no
collation database, normalization engine, locale-tailored multi-code-point case
mapping, locale-sensitive numeric/monetary data, stateful encoding, or
per-thread locale object.

Environment-driven selection, explicit per-category ownership, and composite
`LC_ALL` query/restore are now part of the executable baseline. A C.UTF-8
`LC_CTYPE` no longer causes `LC_ALL` to misreport that unsupported numeric,
time, collation, or monetary locale data exists. The aggregate return string is
opaque and restorable, and invalid aggregate strings fail without mutating the
current process locale. An already wide-oriented FILE still retains its
orientation-time encoding across later process-locale changes.

The next coherent locale promotion is **locale object isolation and reentrant
selection boundaries**. That phase should separate process-global locale state
from an explicit locale object before per-thread adoption, while keeping
collation and locale-sensitive numeric/monetary data outside the contract until
they have real implementation and executable evidence.
