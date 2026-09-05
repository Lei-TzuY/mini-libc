# Formatted output ABI and phase status

The formatted-output engine uses one private parser/conversion core and one
private tagged sink abstraction. `printf` and `fprintf` select the buffered
`FILE` sink; `snprintf` selects a bounded memory sink. Public `vprintf`,
`vfprintf`, and `vsnprintf` normalize caller-created `va_list` state into the
same private argument cursor. No second formatter, fake `FILE`, heap staging
buffer, or raw-write shortcut is used.

## Public surface

`<stdio.h>` exposes:

```c
int printf(const char *restrict format, ...);
int fprintf(FILE *restrict stream, const char *restrict format, ...);
int snprintf(char *restrict s, size_t n, const char *restrict format, ...);
int vprintf(const char *restrict format, va_list ap);
int vfprintf(FILE *restrict stream, const char *restrict format, va_list ap);
int vsnprintf(char *restrict s, size_t n,
              const char *restrict format, va_list ap);
```

All six entry points share the same executable conversion engine. Integer/string
formatting supports `%d`, `%i`, `%u`, `%o`, `%x`, `%X`, `%c`, `%s`, and `%%`;
`-`, `+`, space, `#`, and `0` flags; fixed or `*` width; fixed or `*` precision;
and `hh`, `h`, `l`, and `ll` integer length modifiers.

Floating output now supports the complete binary64 notation families targeted by
this phase: `%f`/`%F`, `%e`/`%E`, `%g`/`%G`, and `%a`/`%A`. A no-length or `l`
modifier consumes the same promoted `double`; `L`/`long double` remains outside
the current ABI.

## Shared floating framing

Every floating conversion first decomposes the binary64 representation into sign,
exponent field, fraction, and finite/infinity/NaN class. Sign selection, special
value spelling, field width, left alignment, and zero padding then pass through
one shared framing path rather than being reimplemented by each notation.

The binary64 sign bit is retained for negative zero. Lowercase conversions spell
special values as `inf` and `nan`; uppercase `%F`, `%E`, `%G`, and `%A` spell
`INF` and `NAN`. Sign flags still apply to infinities and NaNs, while the `0`
flag does not zero-pad a special token.

For finite numeric text, zero padding is inserted after any sign. Hexadecimal
output additionally emits `0x`/`0X` before zero padding, so a field such as
`%015.1a` retains the standard prefix placement rather than burying it after the
padding.

## Fixed decimal `%f` / `%F`

The fixed-point contract from the first floating-output phase remains deliberately
bounded:

- omitted precision means six digits after the decimal point;
- explicit precision from 0 through 9 is supported, including `*` precision;
- `#` retains the decimal point when precision is zero;
- `-`, `+`, space, `0`, fixed width, and `*` width use the shared framing rules;
- finite magnitudes must satisfy `abs(value) < 2^64`;
- larger finite magnitudes or precision above 9 fail deterministically with
  `EOF` and `EINVAL`.

The retained fixed decimal fraction is rounded to nearest with ties to even. The
executable contract keeps the original `2.5 -> 2` and `3.5 -> 4` zero-precision
regressions.

## Scientific `%e` / `%E`

Scientific conversion uses one significant-decimal digit followed by the
requested number of fractional decimal digits and a signed decimal exponent.
Omitted precision is six; explicit precision is bounded to 0 through 9. The
exponent marker follows conversion case and the exponent contains at least two
decimal digits. `#` retains the radix point at zero precision.

The decimal significand is normalized into `[1, 10)` for nonzero values, scaled
to the requested significant-digit count, and rounded to nearest with ties to
even. A carry from `9.99...` into `10.0...` increments the decimal exponent before
text emission.

## General `%g` / `%G`

General conversion interprets precision as significant decimal digits. Omitted
precision is six, an explicit zero precision means one significant digit, and the
implemented maximum is nine.

Notation selection is made **after significand rounding**. Scientific notation is
used when the resulting exponent is less than `-4` or greater than or equal to
the effective precision; otherwise fixed notation is used. This matters at
boundaries such as `9999.5` with `%.4g`, whose rounded representation crosses to
`1e+04`.

Without `#`, trailing fractional zeroes and a now-unnecessary decimal point are
removed. With `#`, the requested significant-digit shape is retained, including
trailing zeroes and the decimal point where required. `%G` selects uppercase
exponent and special-value spelling.

The `%e`/`%g` decimal conversion machinery is a bounded correctness-focused
implementation. It does **not** claim general correctly-rounded binary64-to-decimal
conversion for arbitrary requested precision; the public limit of nine decimal
digits is part of the executable contract.

## Hexadecimal `%a` / `%A`

Hexadecimal formatting is derived directly from the binary64 exponent and
fraction fields rather than by decimal scaling. Normal values use a leading
hexadecimal digit with the unbiased binary exponent; subnormal values use a
leading zero with exponent `-1022`; exact zero uses exponent zero. The exponent
marker is `p` or `P` and always carries an explicit sign.

When precision is omitted, the formatter begins with all 13 hexadecimal fraction
nibbles represented by binary64 and removes only exact trailing zero nibbles. The
result therefore retains the exact binary64 value while avoiding meaningless
zero suffixes. Explicit precision from 0 through 13 is supported. Discarded
fraction bits are rounded to nearest with ties to even at the requested nibble
boundary, including carry into the leading hexadecimal digit.

`#` keeps the radix point at zero precision. `%A` uses `0X`, uppercase hexadecimal
digits, `P`, and uppercase special-value spelling. Precision above 13 is rejected
with `EOF`/`EINVAL` rather than pretending that extra digits carry additional
binary64 information.

## Sink model

The private formatter sink has two modes:

- the FILE sink forwards emitted bytes to `__mini_stdio_write`, preserving the
  existing buffered stream/error behavior used by `printf`, `fprintf`,
  `vprintf`, and `vfprintf`;
- the memory sink stores at most `n - 1` bytes when `n > 0`, never calls the FILE
  output path, and keeps the stored prefix null-terminated after every emit for
  `snprintf` and `vsnprintf`.

The formatter tracks the logical output count independently from the number of
bytes actually stored. Truncation therefore does not stop parsing or counting.
On success, `snprintf` and `vsnprintf` return the full byte count that would have
been produced with sufficient space, excluding the terminating null byte.

For `n > 0`, the destination is null-terminated even when truncation occurs. For
`n == 0`, the destination is never dereferenced and may be null. Passing a null
destination with `n > 0` is a deterministic mini-libc extension that returns
`EOF` and reports `EINVAL`. A logical result outside the positive `int` return
domain is likewise rejected with `EINVAL` rather than wrapping.

## Variadic call boundary

The private formatter cursor carries two independent register lanes plus one
shared overflow-stack cursor:

- up to five remaining INTEGER-class 8-byte GP words;
- up to eight floating 8-byte `double` payloads corresponding to XMM0-XMM7;
- one overflow pointer consumed in source argument order once a class exhausts
  its register allocation.

Ordinary `printf`, `fprintf`, and `snprintf` entry shims capture the remaining GP
slots and the low binary64 payloads from XMM0-XMM7. Under SysV AMD64, `%al`
reports how many vector argument registers were used; the shim clamps that count
to eight and records it in the private cursor.

Public `v*` entries normalize the caller's SysV `va_list`. GP values come from
`reg_save_area + gp_offset`; floating values come from `reg_save_area + fp_offset`
while `fp_offset < 176`, advancing by the 16-byte SysV save slot for each XMM
argument. `overflow_arg_area` is forwarded unchanged. The conversion engine and
sink therefore do not know whether an argument came from ordinary `...`, public
`va_list`, a register, or the overflow stack.

The exact public `va_list` layout and compiler-primitive policy are documented in
`docs/variadic-abi.md`.

## Executable evidence

The freestanding stdio probe keeps every established integer/string/fixed-float
regression and adds deterministic scientific, general, and hexadecimal cases. It
covers default and explicit precision, flags, width, alternate form, uppercase
variants, negative zero, infinity/NaN, `%g` post-rounding notation selection,
trailing-zero suppression, the minimum binary64 subnormal, hexadecimal radix
prefix plus zero padding, and the explicit precision rejection boundaries.

Ordinary `snprintf` and caller-defined `vsnprintf` wrappers both execute the new
notation families. The FILE path executes `%e`, `%g`, and `%a` through a
caller-defined `vfprintf`, flushes the owned stream, rewinds it, and reads back the
exact bytes through mini-libc. Existing nine-double calls continue to force the
ninth floating argument onto the overflow stack.

The pinned tiny-c integration independently compiles and executes ordinary
`snprintf` and caller-created `va_list` `vsnprintf` calls containing `%e`, `%g`,
and `%a`. The same executable runs through GNU `ld` and the pinned
`mini-elf-toolchain`. GCC and Clang run the complete freestanding/runtime suite
and host-libc-independence inspection.

## Phase boundary and next frontier

Floating formatted output breadth is now executable rather than a roadmap item:
all targeted binary64 notation families share the established formatter parser,
XMM/public-`va_list` argument transport, FILE/memory sinks, and common
classification/sign/padding layer. Decimal precision bounds and the stronger
bit-derived hexadecimal contract are explicit, so this phase closes without a
false general dtoa conformance claim.

The next higher architectural frontier is **configurable stream buffering and
buffer ownership**. The current `FILE` layout hard-codes private 256-byte read and
write arrays and exposes no `setvbuf`/`setbuf` policy surface. A coherent next
slice should make full, line, and unbuffered modes executable; define
caller-provided versus libc-owned buffer lifetime; preserve update-stream
read/write barriers, logical positioning, `fflush`, `fclose`, and normal-exit
flush invariants across mode changes; and prove the behavior through deterministic
fake I/O, real owned streams, and the pinned tiny-c/mini-elf path.

`%p`/`%n`, `tmpfile`, C11 exclusive-create modes, wide-character I/O,
locale-sensitive behavior, threading/TLS, long-double formatting, and allocator
tuning remain separate later phases.