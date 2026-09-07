#!/bin/sh
set -eu

: "${MINICC:?set MINICC to a tiny-c-compiler minicc executable}"

LD=${LD:-ld}
OUT=${OUT:-build/tiny-c-integration}

"$MINICC" -nostdinc -Iinclude -c tests/tiny_tls_integration.c \
    -o "$OUT/tls.o"

if [ -n "${MINI_ELF_LINKER:-}" ]; then
    "$MINI_ELF_LINKER" link -o "$OUT/tls" \
        "$OUT/tls.o" "$OUT/crt0.o" "$OUT/libc.a"
    linker_name="mini-elf-toolchain"
else
    "$LD" -static -e _start --build-id=none -o "$OUT/tls" \
        "$OUT/tls.o" "$OUT/crt0.o" "$OUT/libc.a"
    linker_name="GNU ld"
fi

output=$("$OUT/tls")
if [ "$output" != "tiny-native-tls-ok" ]; then
    echo "unexpected tiny-c native TLS output with $linker_name: $output" >&2
    exit 1
fi

./tests/verify-no-host-libc.sh "$OUT/tls"
printf 'tiny-c native TLS integration passed with %s\n' "$linker_name"
