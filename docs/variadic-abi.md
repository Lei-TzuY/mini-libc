# Public variadic ABI and phase status

mini-libc exposes one public `<stdarg.h>` contract together with both output-side
and input-side `v*` stdio entry points. The implementation remains x86-64 SysV
ABI-specific, matching the rest of the current freestanding runtime.

## Public surface

`<stdarg.h>` exposes `va_list`, `va_start`, `va_arg`, `va_copy`, `__va_copy`, and
`va_end`. `<stdio.h>` exposes:

```c
int vprintf(const char *restrict format, va_list ap);
int vfprintf(FILE *restrict stream, const char *restrict format, va_list ap);
int vsnprintf(char *restrict s, size_t n,
              const char *restrict format, va_list ap);
int vscanf(const char *restrict format, va_list ap);
int vfscanf(FILE *restrict stream, const char *restrict format, va_list ap);
int vsscanf(const char *restrict s, const char *restrict format, va_list ap);
```

The ordinary `printf`/`fprintf`/`snprintf` and `scanf`/`fscanf`/`sscanf` entry
points remain available. Ordinary and `v*` output converge on the same formatter
parser and FILE/memory sinks; ordinary and `v*` input converge on the same
scanner parser and FILE/string sources.

## `va_list` call ABI

On GCC and Clang, `<stdarg.h>` uses the compiler's native SysV AMD64 variadic
primitives. On the pinned tiny-c compiler, the fallback `va_list` is deliberately
an array of one four-field SysV state record:

```c
struct {
    unsigned int gp_offset;
    unsigned int fp_offset;
    void *overflow_arg_area;
    void *reg_save_area;
};
```

The array-of-one form is important at public function boundaries: a `va_list`
parameter is adjusted to a pointer to the state record, matching the GCC/Clang
SysV calling convention instead of passing a 24-byte aggregate by value.

The tiny-c fallback passes `&(ap)[0]` to its compiler variadic primitives. This
preserves the public pointer ABI while satisfying the pinned compiler's
pointer-to-state builtin contract.

## Scoped compiler primitive policy

Ordinary production C and headers remain compiler-neutral. The repository's
neutrality audit rejects compiler-specific builtin tokens everywhere in
`include/` and `src/` except for the standard variadic language boundary in
`include/stdarg.h`.

That header is restricted to exactly five compiler primitives:

- `__builtin_va_list`
- `__builtin_va_start`
- `__builtin_va_arg`
- `__builtin_va_copy`
- `__builtin_va_end`

The audit removes only those exact spellings before checking that no other
`__builtin_*` token remains. This is a narrow language-runtime exception, not a
general compiler-specific escape hatch.

## Shared normalization model

The input scanner consumes its established INTEGER-class destination cursor
because every supported scan destination is a pointer. This remains true for
floating input: `%f` receives `float *` and `%lf` receives `double *`; the object
ultimately written contains a floating value, but the variadic argument crossing
the ABI boundary is still a pointer.

The output formatter consumes a richer private cursor with two independent
register lanes plus one shared overflow pointer:

- up to five remaining 8-byte GP words;
- up to eight binary64 floating payloads corresponding to XMM0-XMM7;
- one overflow stack consumed in source argument order once a class exhausts its
  register allocation.

Ordinary formatter entries capture GP arguments exactly as before. They also use
the SysV `%al` vector-register count to record how many XMM argument registers
are live and save the low binary64 payload from XMM0-XMM7 into the private
floating lane. This gives ordinary `printf`, `fprintf`, and `snprintf` the same
FP cursor shape later consumed by the shared formatter.

Public output `v*` adapters normalize both halves of the caller's `va_list`:

- `gp_offset < 48` selects 8-byte GP save slots from `reg_save_area`;
- `fp_offset < 176` selects XMM save slots, each 16 bytes wide, from
  `reg_save_area`; the low 8-byte binary64 payload is compacted into the private
  floating lane;
- `overflow_arg_area` is forwarded unchanged for arguments whose register class
  is exhausted.

The formatter consumes GP and floating values in format order while sharing the
same overflow pointer. This is the critical mixed SysV property: register-backed
arguments do not advance the overflow cursor, while a spilled argument of either
class consumes its stack slot at the point its conversion is processed.

Input `vscanf`/`vfscanf`/`vsscanf` adapters remain GP-only even after floating
formatted input becomes executable. They normalize `gp_offset` and
`overflow_arg_area` into the scanner's existing destination cursor and never
consume `fp_offset`, because no floating scalar is passed by value to the scan
function. Adding `%f`/`%lf` therefore requires no XMM save/copy path on the input
side.

## Executable evidence

The public `<stdarg.h>` baseline remains covered by caller-defined variadic
functions using `va_start`, `va_arg`, `va_copy`, and `va_end`. Integer traversal
crosses the GP register boundary into the overflow stack, and copied lists are
consumed independently.

Output includes real floating transport evidence rather than a dormant ABI shim.
The freestanding stdio probe calls ordinary `snprintf` with nine `double`
arguments so the ninth value spills past XMM0-XMM7. A caller-defined variadic
wrapper repeats the same sequence through `vsnprintf`, proving public `fp_offset`
normalization and floating overflow-stack traversal. Redirected FILE output also
runs floating `printf`, `vprintf`, and a nine-double `vfprintf` path before
reading the exact bytes back.

The first conversion using this transport is the bounded lowercase `%f` surface
documented in `docs/formatted-output.md`. Dynamic width/precision additionally
mix GP arguments with a floating argument in one format, proving that the two
register cursors remain independent while converging on one parser.

Input wrappers use the symmetric pointer path. Existing six-destination
`vscanf`, FILE-backed `vfscanf`, and memory-backed `vsscanf` cases force receiving
pointers across the GP register/overflow boundary. Floating scanner coverage then
passes `float *` and `double *` through the exact same GP cursor across FILE,
memory, and stdin. This proves the important ABI distinction directly: floating
**output values** need XMM transport, while floating **input destinations** do
not.

The pinned tiny-c integration compiles caller-defined wrappers for all six public
`v*` formatted-I/O APIs. It compiles ordinary and public-`va_list` nine-double
formatting calls, floating FILE/memory/stdin scans through `float *`/`double *`,
and a scanned-negative-zero round trip back through `snprintf`. The same binary
executes through GNU `ld` and the pinned mini-elf toolchain. GCC and Clang run the
normal freestanding/runtime suite and host-libc-independence inspection.

## Phase boundary and next frontier

The public variadic core now has executable mixed INTEGER/binary64 transport for
formatted output and a pointer-only INTEGER-class destination path for formatted
input, including floating destinations. Both ordinary variadic calls and
caller-created `va_list` state feed the existing formatter/scanner engines
without compiler-specific `va_arg` use inside production C.

Floating formatted input therefore closes without any scanner FP-register shim:
the newly added work is lexical and numeric conversion, not variadic transport.
The bounded finite decimal/scientific conversion contract is documented in
`docs/formatted-input.md`.

The next architectural frontier is a reusable **public decimal floating
conversion core** for `strtof`/`strtod`. Before exposing that API, the proven
scanner-local conversion machinery should be refactored only alongside a real
standard contract: `endptr` and no-conversion behavior, overflow/underflow,
`inf`/`nan`, and hexadecimal floating syntax. A later scanner `%a`/`%A` surface
can then reuse that core rather than duplicating numeric parsing.

Further floating-output families, `%n`, pointer formatting, wide-character I/O,
locale-sensitive behavior, configurable buffering, `tmpfile`, threading/TLS,
C11 exclusive-create modes, and allocator tuning remain separate later phases.
