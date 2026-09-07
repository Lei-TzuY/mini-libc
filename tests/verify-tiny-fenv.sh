#!/bin/sh
set -eu

: "${MINICC:?set MINICC to a tiny-c-compiler minicc executable}"

CC=${CC:-cc}
AR=${AR:-ar}
LD=${LD:-ld}
OUT=${OUT:-build/tiny-c-integration}

if [ ! -f "$OUT/libc.a" ] || [ ! -f "$OUT/crt0.o" ]; then
    echo "tiny fenv integration requires verify-tiny-c-compiler.sh output" >&2
    exit 1
fi

"$CC" -fno-pie -c src/fenv/fenv_asm.S -o "$OUT/fenv-asm.o"
"$MINICC" -nostdinc -Iinclude -c src/math/fenv_rounding.c \
    -o "$OUT/fenv-rounding.o"
"$MINICC" -nostdinc -Iinclude -c src/math/decompose.c \
    -o "$OUT/fenv-decompose.o"
"$MINICC" -nostdinc -Iinclude -c src/math/explog.c \
    -o "$OUT/fenv-explog.o"
"$AR" rcs "$OUT/libc.a" "$OUT/fenv-asm.o" "$OUT/fenv-rounding.o" \
    "$OUT/fenv-decompose.o" "$OUT/fenv-explog.o"
"$MINICC" -nostdinc -Iinclude -c tests/tiny_fenv_integration.c \
    -o "$OUT/fenv-test.o"

if [ -n "${MINI_ELF_LINKER:-}" ]; then
    "$MINI_ELF_LINKER" link -o "$OUT/fenv-test" \
        "$OUT/fenv-test.o" "$OUT/crt0.o" "$OUT/libc.a"
else
    "$LD" -static -e _start --build-id=none -o "$OUT/fenv-test" \
        "$OUT/fenv-test.o" "$OUT/crt0.o" "$OUT/libc.a"
fi

output=$("$OUT/fenv-test")
if [ "$output" != "tiny-fenv-ok" ]; then
    echo "unexpected tiny-c fenv output: $output" >&2
    exit 1
fi

./tests/verify-no-host-libc.sh "$OUT/fenv-test"

echo "tiny-c floating environment integration passed"
