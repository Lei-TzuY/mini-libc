#!/bin/sh
set -eu

: "${MINICC:?set MINICC to a tiny-c-compiler minicc executable}"

LD=${LD:-ld}
OUT=${OUT:-build/tiny-c-integration}

if [ ! -f "$OUT/libc.a" ] || [ ! -f "$OUT/crt0.o" ]; then
    echo "TLS integration requires the base tiny-c integration build first" >&2
    exit 1
fi

"$MINICC" -nostdinc -Iinclude -c tests/tls_probe.c -o "$OUT/tls-probe.o"
if ! readelf -rW "$OUT/tls-probe.o" | grep -q 'R_X86_64_TPOFF32'; then
    echo "tiny-c TLS probe did not emit R_X86_64_TPOFF32" >&2
    exit 1
fi

if [ -n "${MINI_ELF_LINKER:-}" ]; then
    "$MINI_ELF_LINKER" link -o "$OUT/tls-probe" \
        "$OUT/tls-probe.o" "$OUT/crt0.o" "$OUT/libc.a"
    linker_name="mini-elf-toolchain"
else
    "$LD" -static -e _start --build-id=none -o "$OUT/tls-probe" \
        "$OUT/tls-probe.o" "$OUT/crt0.o" "$OUT/libc.a"
    linker_name="GNU ld"
fi

if ! readelf -lW "$OUT/tls-probe" | grep -q ' TLS '; then
    echo "$linker_name output is missing PT_TLS" >&2
    exit 1
fi

output=$("$OUT/tls-probe")
if [ "$output" != "tls-ok" ]; then
    echo "unexpected $linker_name TLS runtime output: $output" >&2
    exit 1
fi

./tests/verify-no-host-libc.sh "$OUT/tls-probe"
echo "tiny-c _Thread_local -> mini-libc -> $linker_name TLS integration passed"
