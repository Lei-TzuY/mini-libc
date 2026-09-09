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
"$MINICC" -nostdinc -Iinclude -c src/math/hyperbolic.c \
    -o "$OUT/math_hyperbolic.o"
"$MINICC" -nostdinc -Iinclude -c src/math/trig.c \
    -o "$OUT/math_trig.o"
"$MINICC" -nostdinc -Iinclude -c src/math/special.c \
    -o "$OUT/math_special.o"
"$AR" rcs "$OUT/libc.a" "$OUT/fenv-asm.o" "$OUT/fenv-rounding.o" \
    "$OUT/fenv-decompose.o" "$OUT/fenv-explog.o" "$OUT/math_hyperbolic.o" \
    "$OUT/math_trig.o" "$OUT/math_special.o"
"$MINICC" -nostdinc -Iinclude -c tests/tiny_fenv_integration.c \
    -o "$OUT/fenv-test.o"
"$MINICC" -nostdinc -Iinclude -c tests/tiny_fenv_hyperbolic.c \
    -o "$OUT/fenv-hyperbolic-test.o"
"$MINICC" -nostdinc -Iinclude -c tests/tiny_fenv_gamma.c \
    -o "$OUT/fenv-gamma-test.o"
"$MINICC" -nostdinc -Iinclude -c tests/tiny_fenv_trig.c \
    -o "$OUT/fenv-trig-test.o"
"$MINICC" -nostdinc -Iinclude -c tests/tiny_fenv_special.c \
    -o "$OUT/fenv-special-test.o"

if [ -n "${MINI_ELF_LINKER:-}" ]; then
    "$MINI_ELF_LINKER" link -o "$OUT/fenv-test" \
        "$OUT/fenv-test.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/fenv-hyperbolic-test" \
        "$OUT/fenv-hyperbolic-test.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/fenv-gamma-test" \
        "$OUT/fenv-gamma-test.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/fenv-trig-test" \
        "$OUT/fenv-trig-test.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/fenv-special-test" \
        "$OUT/fenv-special-test.o" "$OUT/crt0.o" "$OUT/libc.a"
else
    "$LD" -static -e _start --build-id=none -o "$OUT/fenv-test" \
        "$OUT/fenv-test.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/fenv-hyperbolic-test" \
        "$OUT/fenv-hyperbolic-test.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/fenv-gamma-test" \
        "$OUT/fenv-gamma-test.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/fenv-trig-test" \
        "$OUT/fenv-trig-test.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/fenv-special-test" \
        "$OUT/fenv-special-test.o" "$OUT/crt0.o" "$OUT/libc.a"
fi

set +e
output=$("$OUT/fenv-test")
status=$?
set -e
if [ "$status" -ne 0 ] || [ "$output" != "tiny-fenv-ok" ]; then
    echo "unexpected tiny-c fenv: status=$status output='$output'" >&2
    exit 1
fi

set +e
output=$("$OUT/fenv-hyperbolic-test")
status=$?
set -e
if [ "$status" -ne 0 ] || [ "$output" != "tiny-fenv-hyperbolic-ok" ]; then
    echo "unexpected tiny-c hyperbolic fenv: status=$status output='$output'" >&2
    exit 1
fi

set +e
output=$("$OUT/fenv-gamma-test")
status=$?
set -e
if [ "$status" -ne 0 ] || [ "$output" != "tiny-fenv-gamma-ok" ]; then
    echo "unexpected tiny-c gamma fenv: status=$status output='$output'" >&2
    exit 1
fi

set +e
output=$("$OUT/fenv-trig-test")
status=$?
set -e
if [ "$status" -ne 0 ] || [ "$output" != "tiny-fenv-trig-ok" ]; then
    echo "unexpected tiny-c trig fenv: status=$status output='$output'" >&2
    exit 1
fi

set +e
output=$("$OUT/fenv-special-test")
status=$?
set -e
if [ "$status" -ne 0 ] || [ "$output" != "tiny-fenv-special-ok" ]; then
    echo "unexpected tiny-c special fenv: status=$status output='$output'" >&2
    exit 1
fi

./tests/verify-no-host-libc.sh "$OUT/fenv-test" "$OUT/fenv-hyperbolic-test" \
    "$OUT/fenv-gamma-test" "$OUT/fenv-trig-test" "$OUT/fenv-special-test"

echo "tiny-c floating environment integration passed"
