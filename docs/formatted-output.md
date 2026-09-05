# Formatted output ABI and phase status

The formatted-output engine uses one private parser/conversion core and one
private tagged sink abstraction. `printf` and `fprintf` select the existing
buffered `FILE` sink; `snprintf` selects a bounded memory sink. The public
`vprintf`, `vfprintf`, and `vsnprintf` entries feed caller-created `va_list`
state into those same paths. No second formatter, fake `FILE`, heap staging
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

The first floating-output slice adds lowercase `%f` to that same engine. It is a
deliberately bounded fixed-point implementation rather than a claim of complete
C floating formatting:

- the argument is binary64 `double`; an `l` modifier is accepted with the same
  meaning as `%f`, while `L`/`long double` is not implemented;
- omitted precision means six digits after the decimal point;
- explicit precision from 0 through 9 is supported, including `*` precision;
- `-`, `+`, space, `#`, `0`, fixed width, and `*` width use the shared formatter
  padding/sign rules;
- `#` keeps the decimal point when precision is zero;
- the binary64 sign bit is preserved for negative zero;
- infinities and NaNs are recognized from their IEEE-754 representation and
  emitted as lowercase `inf` and `nan` tokens;
- finite values must satisfy `abs(value) < 2^64`; larger finite magnitudes are
  rejected with the existing deterministic `EOF`/`EINVAL` policy;
- precision above 9 is likewise rejected instead of silently pretending that a
  general-purpose dtoa implementation exists.

The fixed conversion rounds the retained decimal fraction to nearest with ties
to even. The executable contract includes both `2.5 -> 2` and `3.5 -> 4` at zero
precision. `%e`, `%E`, `%g`, `%G`, `%a`, `%A`, locale-dependent formatting, and
long-double formatting remain outside this phase.

## Sink model

The private formatter sink has two modes:

- the FILE sink forwards emitted bytes to `__mini_stdio_write`, preserving the
  existing buffered stream/error behavior used by `printf`, `fprintf`,
  `vprintf`, and `vfprintf`;
- the memory sink stores at most `n - 1` bytes when `n > 0`, never calls the FILE
  output path, and keeps the stored prefix null-terminated after every emit for
  `snprintf` and `vsnprintf`.

The formatter tracks the logical output count independently from the number of
bytes actually stored in the bounded memory sink. Truncation therefore does not
stop parsing or counting. On successful formatting, `snprintf` and `vsnprintf`
return the full number of bytes that would have been produced with sufficient
space, excluding the terminating null byte.

For `n > 0`, the destination is null-terminated even when truncation occurs. For
`n == 0`, the destination is never dereferenced and may be null. Passing a null
destination with `n > 0` is a deterministic mini-libc extension that returns
`EOF` and reports `EINVAL`.

As with the FILE formatter, a logical result that cannot fit in the positive
`int` return range is rejected deterministically with `EINVAL` rather than
silently wrapping the return count.

## Variadic call boundary

The private formatter cursor now carries two independent register lanes plus one
shared overflow-stack cursor:

- up to five remaining INTEGER-class 8-byte GP words;
- up to eight floating 8-byte `double` payloads corresponding to XMM0-XMM7;
- one overflow pointer consumed in source argument order once a class exhausts
  its register allocation.

Ordinary `printf`, `fprintf`, and `snprintf` entry shims retain the GP captures
used by the integer formatter and additionally save the low binary64 payload from
XMM0-XMM7. Under the SysV AMD64 variadic calling convention, `%al` tells the
callee how many vector argument registers were used; the shim clamps that count
to eight and records it in the private cursor. The first stack argument remains
available through the same overflow cursor.

The public `v*` entries normalize the caller's SysV `va_list`. GP values still
come from `reg_save_area + gp_offset`. Floating values come from
`reg_save_area + fp_offset` while `fp_offset < 176`, advancing by the SysV
16-byte save slot for each XMM argument. The adapter compacts each slot's low
binary64 payload into the private floating lane and forwards
`overflow_arg_area` unchanged. The parser and sink therefore do not know whether
an argument originated in ordinary `...`, a public `va_list`, a GP register, an
XMM register, or the overflow stack.

The exact public `va_list` layout and compiler-primitive policy are documented in
`docs/variadic-abi.md`.

## Executable evidence

The freestanding stdio probe retains all integer/string formatting regressions
and adds floating evidence across memory and FILE sinks. It verifies default and
explicit fixed precision, `l`, sign/space/alternate/zero/left flags, dynamic
width and precision, negative zero, infinities, NaN, nearest-even half cases,
precision rejection, and the bounded finite-magnitude contract.

Ordinary `snprintf` receives nine `double` values in one call, forcing the ninth
floating argument past XMM0-XMM7 onto the overflow stack. A caller-defined
variadic wrapper repeats the same nine-value path through `vsnprintf` and public
`va_list` state. Redirected FILE tests execute floating `printf`, `vprintf`, and
a nine-double `vfprintf` path and read the exact bytes back through mini-libc.

The pinned tiny-c integration independently compiles both an ordinary nine-double
`snprintf` call and a caller-created `va_list` nine-double `vsnprintf` call, then
produces the final status line through floating `vprintf`. The same executable is
linked and run through GNU `ld` and the pinned `mini-elf-toolchain`. GCC and
Clang run the complete freestanding/runtime suite and host-libc-independence
inspection.

## Phase boundary and next frontier

The formatter has an executable **GP + floating SysV variadic baseline**:
ordinary variadic calls and public `va_list` calls share one argument cursor,
one parser/conversion engine, and the same FILE/memory sinks. Lowercase bounded
`%f` proves that floating transport is consumed by real formatting behavior,
not merely captured by an unused ABI shim.

Floating input and public string conversion have now moved ahead of the output
surface: the scanner and `strtof`/`strtod` share a dedicated decimal/hex/special
conversion engine, while output still exposes only bounded fixed `%f`. The next
higher-value architectural frontier is therefore **floating formatted output
breadth**.

A coherent slice should add `%e`/`%E`, `%g`/`%G`, and `%a`/`%A` through reusable
binary64 decomposition, rounding, exponent selection, and notation-selection
logic inside the existing formatter. It must preserve ordinary/public-`va_list`
XMM transport, FILE/memory sinks, width/precision/flag behavior, special values,
and pinned GCC/Clang/tiny-c/mini-elf execution. As with the input converter, the
implementation must state its rounding bounds explicitly rather than presenting
a partial dtoa algorithm as general correctly-rounded conversion.

`%n`, pointer formatting, wide-character I/O, locale-sensitive behavior,
configurable buffering, `tmpfile`, threading/TLS, C11 exclusive-create modes,
long-double formatting, and allocator tuning remain separate later phases.
