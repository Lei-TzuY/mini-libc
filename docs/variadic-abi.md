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

The established formatter and scanner each consume the same-shaped private GP
cursor: up to five remaining 8-byte register words, an overflow-stack pointer,
a current register index, and a register count. Public `v*` entry points therefore
do not fork either parser and do not call `va_arg` inside production C.

Their SysV AMD64 assembly adapters read the public `va_list` state, copy the
remaining INTEGER-class slots from `reg_save_area + gp_offset` into the existing
private cursor, carry `overflow_arg_area` forward unchanged, reset the private
cursor index, and enter the existing formatter or scanner dispatch.

Output adapters feed integer values and pointers into the existing formatter.
Input adapters feed receiving pointers into the existing scanner destination
cursor. Because every currently supported formatted-I/O argument is
INTEGER-class, this normalization is sufficient for the executable surface.
`fp_offset` remains part of the public state layout but is intentionally not
consumed yet. No floating-point, vector, long-double, or aggregate variadic
formatting/scanning claim is made by this phase.

## Executable evidence

The freestanding output probe verifies caller-defined variadic functions using
`va_start`, `va_arg`, `va_copy`, and `va_end`; integer traversal crosses from the
remaining SysV GP register slots into the overflow stack. A copied `va_list` is
consumed independently through two `vsnprintf` calls and produces identical
output.

Output wrappers exercise different named-argument shapes: `vsnprintf` reaches
stack-resident values after three named GP arguments, `vfprintf` after two, and
`vprintf` after one. FILE-backed output is read back byte-for-byte and bounded
memory output retains the established truncation and logical-count semantics.

Input wrappers exercise the symmetric destination path. The deterministic fake
scanner routes its six-destination stdin case through `vscanf`, forcing the sixth
receiving pointer through `overflow_arg_area`. The same harness routes a
FILE-backed case through `vfscanf` and a memory-source case through `vsscanf`
without changing the established matching, EOF, cached-input, or read-isolation
expectations.

The freestanding scanner probe drives `vfscanf` with eight receiving pointers,
so multiple destinations are necessarily stack-resident, then exercises
`vsscanf` and `vscanf` while retaining the existing real FILE, memory source,
stdin, errno, matching-failure, and EOF evidence.

The pinned tiny-c integration compiles caller-defined wrappers for all six public
`v*` formatted-I/O APIs. `vfscanf` and `vsscanf` reuse the existing file/string
integration; `vscanf` receives six integers from real stdin so the sixth
destination crosses the GP-register boundary. The same executable then runs
through GNU `ld` and the pinned mini-elf toolchain. GCC and Clang run the normal
freestanding/runtime suite and host-libc-independence inspection.

## Phase boundary and next frontier

The public variadic core is now symmetric and executable: caller-created
`va_list` state can cross the public library boundary into the existing shared
formatter and scanner without duplicating either parser or weakening FILE/memory
source/sink semantics.

The next architectural frontier is **floating-point variadic transport and the
first formatted floating-output slice**. Before adding `%f`-family behavior, the
private argument model must grow beyond INTEGER-class words: ordinary variadic
entries and public `va_list` adapters need deterministic SysV XMM/register-save
handling, with executable GCC/Clang/tiny-c/mini-elf evidence. Only after that ABI
transport is proven should a first floating conversion be admitted into the
shared formatter. This must be a real conversion path, not an API placeholder or
a claim based only on compile success.

`%n`, pointer formatting, wide-character I/O, locale-sensitive behavior,
configurable buffering, `tmpfile`, threading/TLS, C11 exclusive-create modes,
and allocator tuning remain separate later phases.
