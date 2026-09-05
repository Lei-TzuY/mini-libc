#!/bin/sh
set -eu

CC=${CC:-cc}
OUT=build/buffering-test
CFLAGS='-std=c11 -O2 -Wall -Wextra -Werror -pedantic -fno-builtin -fno-pie'

rm -rf "$OUT"
mkdir -p "$OUT"

$CC -Iinclude $CFLAGS \
    -Dmini_sys_read=mini_test_read -Dmini_sys_write=mini_test_write \
    -c src/stdio/stdio.c -o "$OUT/stdio.o"
$CC -Iinclude $CFLAGS \
    -Dmini_sys_openat=mini_test_openat -Dmini_sys_close=mini_test_close \
    -Dmalloc=mini_test_malloc -Dfree=mini_test_free \
    -c src/stdio/file.c -o "$OUT/file.o"
$CC -Iinclude $CFLAGS -c src/stdio/file_sync.c -o "$OUT/file_sync.o"
$CC -Iinclude $CFLAGS -c src/stdio/position.c -o "$OUT/position.o"
$CC -Iinclude $CFLAGS -c tests/stdio_buffering_test.c -o "$OUT/test.o"
$CC -Iinclude $CFLAGS -c src/errno/errno.c -o "$OUT/errno.o"
$CC $CFLAGS -c tests/stdio_lock_fake.c -o "$OUT/stdio_lock_fake.o"
$CC -no-pie -o "$OUT/test" "$OUT/test.o" "$OUT/stdio.o" "$OUT/file.o" \
    "$OUT/file_sync.o" "$OUT/position.o" "$OUT/errno.o" "$OUT/stdio_lock_fake.o"

"$OUT/test"
