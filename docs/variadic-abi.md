# Public variadic ABI and phase status

mini-libc now exposes a public `<stdarg.h>` contract together with the output-side
`vprintf`, `vfprintf`, and `vsnprintf` entry points. The implementation remains
x86-64 SysV ABI-specific, matching the rest of the current freestanding runtime.

## Public surface

`<stdarg.h>` exposes `va_list`, `va_start`, `va_arg`, `va_copy`, `__va_copy`, and
`va_end`. `<stdio.h>` exposes:

```c
int vprintf(const char *restrict format, va_list ap);
int vfprintf(FILE *restrict stream, const char *restrict format, va_list ap);
int vsnprintf(char *restrict s, size_t n,
              const char *restrict format, va_list ap);
```

The ordinary `printf`, `fprintf`, and `snprintf` entry points remain unchanged.
Both ordinary and `v*` entry points converge on the same formatter parser,
conversion engine, return-count rules, FILE sink, and bounded-memory sink.

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
SysV calling convention rather than passing a 24-byte aggregate by value.

The tiny-c fallback passes `&(ap)[0]` to its compiler variadic primitives. This
preserves the public pointer ABI while satisfying the pinned compiler's
pointer-to-state builtin contract.

## Scoped compiler primitive policy

Ordinary production C and headers remain compiler-neutral. The repository's
neutrality audit still rejects compiler-specific builtin tokens everywhere in
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

## Formatter normalization

The established formatter already consumes a private `mini_format_args` cursor
containing up to five remaining GP-register words plus an overflow-stack
pointer. `vprintf`, `vfprintf`, and `vsnprintf` therefore do not fork the
formatter and do not use `va_arg` internally.

Their SysV AMD64 assembly adapters read the public `va_list` state, copy the
remaining INTEGER-class register slots from `reg_save_area + gp_offset` into the
existing private cursor, carry `overflow_arg_area` forward unchanged, reset the
private cursor index, and enter the existing formatter dispatch.

Because every currently supported formatted-output conversion consumes only
INTEGER-class values or pointers, this normalization is sufficient for the
current executable surface. `fp_offset` is intentionally preserved as part of
the public state layout but is not consumed yet. This phase makes no claim of
floating-point, vector, long-double, or aggregate variadic formatting support.

## Executable evidence

The freestanding probe verifies caller-defined variadic functions using
`va_start`, `va_arg`, `va_copy`, and `va_end`; integer traversal crosses from the
five remaining SysV GP register slots into the overflow stack. A copied
`va_list` is consumed independently through two `vsnprintf` calls and must
produce identical output.

Separate wrappers exercise different named-argument shapes:

- `vsnprintf`: three named GP arguments, so the fourth and later variadic GP
  values are stack-resident;
- `vfprintf`: two named GP arguments, so the fifth variadic GP value is
  stack-resident;
- `vprintf`: one named GP argument, so the sixth variadic GP value is
  stack-resident.

The FILE-backed `vfprintf`/`vprintf` probe writes to an owned temporary stream and
reads the exact bytes back. The memory-backed `vsnprintf` probe uses the already
validated bounded sink and logical return-count contract.

The pinned tiny-c integration compiles caller-defined variadic wrappers, executes
public `va_arg` and `va_copy`, runs all three output-side `v*` APIs, preserves the
external file content contract, and then executes the same integration binary
through the pinned mini-elf toolchain. GCC and Clang run the same freestanding
runtime/probe suite and host-libc-independence inspection.

## Phase boundary and next frontier

The output side of the public variadic core is now an executable baseline:
caller-created `va_list` state can cross the public library boundary and feed the
same formatter used by the ordinary variadic entry points.

The next coherent frontier is the **input-side public variadic core**:
`vscanf`, `vfscanf`, and `vsscanf` should normalize the same public SysV
`va_list` representation into the existing scanner destination cursor without
forking the scanner parser. That phase must retain deterministic matching/input
failure behavior, FILE/memory source isolation, GP-register/overflow-stack
evidence, GCC/Clang/tiny-c compatibility, and mini-elf execution.

Floating-point formatting/scanning, `%n`, wide-character I/O, locale-sensitive
behavior, configurable buffering, `tmpfile`, threading/TLS, and C11
exclusive-create modes remain separate later phases.
