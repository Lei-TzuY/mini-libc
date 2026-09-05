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
"$CC" -fno-pie -c src/internal/atomic.S -o "$OUT/atomic.o"
"$CC" -fno-pie -c src/stdio/format_entry.S -o "$OUT/format_entry.o"
"$CC" -fno-pie -c src/stdio/scan_entry.S -o "$OUT/scan_entry.o"
"$CC" -fno-pie -c src/control/setjmp.S -o "$OUT/setjmp.o"
"$CC" -fno-pie -c src/thread/thread_entry.S -o "$OUT/thread-entry.o"
"$CC" -fno-pie -c src/crt/crt0.S -o "$OUT/crt0.o"
"$AR" rcs "$OUT/libc.a" $objects "$OUT/syscall.o" "$OUT/atomic.o" \
    "$OUT/format_entry.o" "$OUT/scan_entry.o" "$OUT/setjmp.o" \
    "$OUT/thread-entry.o"

"$MINICC" -nostdinc -Iinclude -c tests/tiny_c_integration.c \
    -o "$OUT/integration.o"
"$MINICC" -nostdinc -Iinclude -c tests/tiny_buffering_integration.c \
    -o "$OUT/buffering.o"
"$MINICC" -nostdinc -Iinclude -c tests/tiny_pathname_integration.c \
    -o "$OUT/pathname.o"
"$MINICC" -nostdinc -Iinclude -c tests/tiny_rebind_integration.c \
    -o "$OUT/rebind.o"
"$MINICC" -nostdinc -Iinclude -c tests/tiny_time_integration.c \
    -o "$OUT/time.o"
"$MINICC" -nostdinc -Iinclude -c tests/tiny_termination_integration.c \
    -o "$OUT/termination.o"
"$MINICC" -nostdinc -Iinclude -c tests/tiny_setjmp_integration.c \
    -o "$OUT/setjmp-test.o"
"$MINICC" -nostdinc -Iinclude -c tests/tiny_thread_integration.c \
    -o "$OUT/thread.o"

if [ -n "${MINI_ELF_LINKER:-}" ]; then
    "$MINI_ELF_LINKER" link -o "$OUT/integration" \
        "$OUT/integration.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/buffering" \
        "$OUT/buffering.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/pathname" \
        "$OUT/pathname.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/rebind" \
        "$OUT/rebind.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/time" \
        "$OUT/time.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/termination" \
        "$OUT/termination.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/setjmp-test" \
        "$OUT/setjmp-test.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$MINI_ELF_LINKER" link -o "$OUT/thread" \
        "$OUT/thread.o" "$OUT/crt0.o" "$OUT/libc.a"
    linker_name="mini-elf-toolchain"
else
    "$LD" -static -e _start --build-id=none -o "$OUT/integration" \
        "$OUT/integration.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/buffering" \
        "$OUT/buffering.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/pathname" \
        "$OUT/pathname.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/rebind" \
        "$OUT/rebind.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/time" \
        "$OUT/time.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/termination" \
        "$OUT/termination.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/setjmp-test" \
        "$OUT/setjmp-test.o" "$OUT/crt0.o" "$OUT/libc.a"
    "$LD" -static -e _start --build-id=none -o "$OUT/thread" \
        "$OUT/thread.o" "$OUT/crt0.o" "$OUT/libc.a"
    linker_name="GNU ld"
fi

io_path="$OUT/owned-file.tmp"
rm -f "$io_path"
output=$(printf '1 2 3 4 5 6 1.5 -2.5e2' | \
    MINI_TINY_C=yes MINI_IO_PATH="$io_path" "$OUT/integration" arg)
if [ "$output" != "tiny-c-integration-ok:+00007:0x2a:-5000000000:11:22:33:1.5" ]; then
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

buffering_path="$OUT/buffering-file.tmp"
rm -f "$buffering_path"
buffering_output=$("$OUT/buffering" "$buffering_path")
if [ "$buffering_output" != "tiny-buffering-ok" ]; then
    echo "unexpected tiny-c buffering output: $buffering_output" >&2
    rm -f "$buffering_path"
    exit 1
fi
expected_buffering=$(printf 'abcdeF\nGHI')
if [ "$(cat "$buffering_path")" != "$expected_buffering" ]; then
    echo "unexpected tiny-c buffering file contents" >&2
    rm -f "$buffering_path"
    exit 1
fi
rm -f "$buffering_path"

pathname_source="$OUT/pathname-source.tmp"
pathname_target="$OUT/pathname-target.tmp"
pathname_dir="$OUT/pathname-dir.tmp"
rm -f "$pathname_source" "$pathname_target"
rmdir "$pathname_dir" 2>/dev/null || true
mkdir "$pathname_dir"
pathname_output=$("$OUT/pathname" "$pathname_source" "$pathname_target" "$pathname_dir")
if [ "$pathname_output" != "tiny-pathname-ok" ]; then
    echo "unexpected tiny-c pathname output: $pathname_output" >&2
    rm -f "$pathname_source" "$pathname_target"
    rmdir "$pathname_dir" 2>/dev/null || true
    exit 1
fi
if [ -e "$pathname_source" ] || [ -e "$pathname_target" ] || [ -e "$pathname_dir" ]; then
    echo "tiny-c pathname integration left filesystem state behind" >&2
    rm -f "$pathname_source" "$pathname_target"
    rmdir "$pathname_dir" 2>/dev/null || true
    exit 1
fi

rebind_old="$OUT/rebind-old.tmp"
rebind_new="$OUT/rebind-new.tmp"
rm -f "$rebind_old" "$rebind_new"
rebind_output=$("$OUT/rebind" "$rebind_old" "$rebind_new")
if [ "$rebind_output" != "tiny-rebind-ok" ]; then
    echo "unexpected tiny-c rebind output: $rebind_output" >&2
    rm -f "$rebind_old" "$rebind_new"
    exit 1
fi
if [ "$(cat "$rebind_old")" != "OLD" ] || [ "$(cat "$rebind_new")" != "NEW" ]; then
    echo "unexpected tiny-c rebind file contents" >&2
    rm -f "$rebind_old" "$rebind_new"
    exit 1
fi
rm -f "$rebind_old" "$rebind_new"

time_output=$("$OUT/time")
if [ "$time_output" != "tiny-time-ok" ]; then
    echo "unexpected tiny-c time output: $time_output" >&2
    exit 1
fi

setjmp_output=$("$OUT/setjmp-test")
if [ "$setjmp_output" != "tiny-setjmp-ok" ]; then
    echo "unexpected tiny-c setjmp output: $setjmp_output" >&2
    exit 1
fi

thread_output=$("$OUT/thread")
if [ "$thread_output" != "tiny-threads-ok" ]; then
    echo "unexpected tiny-c thread output: $thread_output" >&2
    exit 1
fi

set +e
termination_quick_output=$("$OUT/termination" quick)
termination_quick_status=$?
set -e
if [ "$termination_quick_status" -ne 41 ] || [ "$termination_quick_output" != 21 ]; then
    echo "unexpected tiny-c quick termination: status=$termination_quick_status output='$termination_quick_output'" >&2
    exit 1
fi

set +e
termination_abort_output=$("$OUT/termination" abort 2>/dev/null)
termination_abort_status=$?
set -e
if [ "$termination_abort_status" -ne 134 ] || [ "$termination_abort_output" != H ]; then
    echo "unexpected tiny-c abort termination: status=$termination_abort_status output='$termination_abort_output'" >&2
    exit 1
fi

./tests/verify-no-host-libc.sh "$OUT/integration"
./tests/verify-no-host-libc.sh "$OUT/buffering"
./tests/verify-no-host-libc.sh "$OUT/pathname"
./tests/verify-no-host-libc.sh "$OUT/rebind"
./tests/verify-no-host-libc.sh "$OUT/time"
./tests/verify-no-host-libc.sh "$OUT/termination"
./tests/verify-no-host-libc.sh "$OUT/setjmp-test"
./tests/verify-no-host-libc.sh "$OUT/thread"

echo "tiny-c-compiler -> mini-libc -> $linker_name integration passed"
