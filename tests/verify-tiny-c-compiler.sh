#!/bin/sh
set -eu

: "${MINICC:?set MINICC to a tiny-c-compiler minicc executable}"

CC=${CC:-cc}
AR=${AR:-ar}
LD=${LD:-ld}
OUT=${OUT:-build/tiny-c-integration}

rm -rf "$OUT"
mkdir -p "$OUT/obj"

objects=""
for source in $(find src -type f -name '*.c' -print | sort); do
    name=$(printf '%s' "$source" | tr '/.' '__')
    object="$OUT/obj/$name.o"
    "$MINICC" -nostdinc -Iinclude -c "$source" -o "$object"
    objects="$objects $object"
done

"$CC" -fno-pie -c src/syscall/syscall.S -o "$OUT/syscall.o"
"$CC" -fno-pie -c src/crt/crt0.S -o "$OUT/crt0.o"
"$AR" rcs "$OUT/libc.a" $objects "$OUT/syscall.o"

"$MINICC" -nostdinc -Iinclude -c tests/tiny_c_integration.c \
    -o "$OUT/integration.o"
"$LD" -static -e _start --build-id=none -o "$OUT/integration" \
    "$OUT/integration.o" "$OUT/crt0.o" "$OUT/libc.a"

output=$(MINI_TINY_C=yes "$OUT/integration" arg)
if [ "$output" != "tiny-c-integration-ok" ]; then
    echo "unexpected tiny-c-compiler integration output: $output" >&2
    exit 1
fi

./tests/verify-no-host-libc.sh "$OUT/integration"

echo "tiny-c-compiler -> mini-libc compile/link/runtime integration passed"
