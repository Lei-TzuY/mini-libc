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
"$CC" -fno-pie -c src/stdio/format_entry.S -o "$OUT/format_entry.o"
"$CC" -fno-pie -c src/stdio/scan_entry.S -o "$OUT/scan_entry.o"
"$CC" -fno-pie -c src/crt/crt0.S -o "$OUT/crt0.o"
"$AR" rcs "$OUT/libc.a" $objects "$OUT/syscall.o" "$OUT/format_entry.o" \
    "$OUT/scan_entry.o"

"$MINICC" -nostdinc -Iinclude -c tests/tiny_c_integration.c \
    -o "$OUT/integration.o"

if [ -n "${MINI_ELF_LINKER:-}" ]; then
    "$MINI_ELF_LINKER" link -o "$OUT/integration" \
        "$OUT/integration.o" "$OUT/crt0.o" "$OUT/libc.a"
    linker_name="mini-elf-toolchain"
else
    "$LD" -static -e _start --build-id=none -o "$OUT/integration" \
        "$OUT/integration.o" "$OUT/crt0.o" "$OUT/libc.a"
    linker_name="GNU ld"
fi

io_path="$OUT/owned-file.tmp"
rm -f "$io_path"
output=$(MINI_TINY_C=yes MINI_IO_PATH="$io_path" "$OUT/integration" arg)
if [ "$output" != "tiny-c-integration-ok:+00007:0x2a:-5000000000:11:22:33" ]; then
    echo "unexpected tiny-c-compiler integration output: $output" >&2
    rm -f "$io_path"
    exit 1
fi
if [ "$(cat "$io_path")" != "012345XY89" ]; then
    echo "unexpected tiny-c-compiler positioned file contents" >&2
    rm -f "$io_path"
    exit 1
fi
rm -f "$io_path"

./tests/verify-no-host-libc.sh "$OUT/integration"

echo "tiny-c-compiler -> mini-libc -> $linker_name integration passed"
