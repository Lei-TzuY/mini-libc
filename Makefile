CC ?= cc
AR ?= ar
LD ?= ld

CPPFLAGS := -Iinclude
CFLAGS := -std=c11 -O2 -Wall -Wextra -Werror -pedantic -ffreestanding \
          -fno-builtin -fno-stack-protector -fno-pie
ASFLAGS := -fno-pie
HOST_CFLAGS := -std=c11 -O2 -Wall -Wextra -Werror -pedantic \
               -fno-builtin -fno-pie
HOST_LDFLAGS := -no-pie
MEMORY_RENAMES := -Dmemcpy=mini_test_memcpy -Dmemmove=mini_test_memmove \
                  -Dmemset=mini_test_memset -Dmemcmp=mini_test_memcmp \
                  -Dmemchr=mini_test_memchr
STRING_RENAMES := -Dstrlen=mini_test_strlen -Dstrcmp=mini_test_strcmp \
                  -Dstrncmp=mini_test_strncmp -Dstrcpy=mini_test_strcpy \
                  -Dstrncpy=mini_test_strncpy -Dstrchr=mini_test_strchr \
                  -Dstrrchr=mini_test_strrchr -Dstrstr=mini_test_strstr \
                  -Dstrspn=mini_test_strspn -Dstrcspn=mini_test_strcspn \
                  -Dstrpbrk=mini_test_strpbrk -Dstrcat=mini_test_strcat \
                  -Dstrncat=mini_test_strncat -Dstrtok=mini_test_strtok
ATOI_RENAMES := -Datoi=mini_test_atoi
STRTOL_RENAMES := -Dstrtol=mini_test_strtol
STRTOUL_RENAMES := -Dstrtoul=mini_test_strtoul
STRTOD_RENAMES := -Dstrtod=mini_test_strtod -Dstrtof=mini_test_strtof
BSEARCH_RENAMES := -Dbsearch=mini_test_bsearch
ALLOCATOR_RENAMES := -Dmalloc=mini_test_malloc -Drealloc=mini_test_realloc \
                     -Dfree=mini_test_free -Dmini_sys_brk=mini_test_brk
STDIO_RENAMES := -Dmini_sys_read=mini_test_read -Dmini_sys_write=mini_test_write
FILE_RENAMES := -Dmini_sys_openat=mini_test_openat -Dmini_sys_close=mini_test_close \
                -Dmalloc=mini_test_malloc -Dfree=mini_test_free
BLOCK_RENAMES := -Dmini_sys_read=mini_test_read -Dmini_sys_write=mini_test_write
POSITION_RENAMES := -Dmini_sys_lseek=mini_test_lseek

BUILD := build
LIBC := $(BUILD)/libc.a
CRT0 := $(BUILD)/crt0.o
LIB_OBJS := $(BUILD)/start.o $(BUILD)/termination.o $(BUILD)/syscall.o \
            $(BUILD)/memory.o $(BUILD)/string.o $(BUILD)/strerror.o \
            $(BUILD)/ctype.o $(BUILD)/atoi.o $(BUILD)/strtol.o \
            $(BUILD)/strtoul.o $(BUILD)/strtod.o $(BUILD)/bsearch.o \
            $(BUILD)/allocator.o $(BUILD)/calloc.o $(BUILD)/getenv.o \
            $(BUILD)/stdio.o $(BUILD)/format.o $(BUILD)/format_entry.o \
            $(BUILD)/scan.o $(BUILD)/float_parse.o $(BUILD)/scan_entry.o \
            $(BUILD)/file_stream.o $(BUILD)/block_io.o $(BUILD)/position.o \
            $(BUILD)/errno.o
PROGRAMS := $(BUILD)/hello $(BUILD)/runtime_probe $(BUILD)/syscall_probe \
            $(BUILD)/memory_probe $(BUILD)/string_probe $(BUILD)/strtok_probe \
            $(BUILD)/strerror_probe $(BUILD)/ctype_probe $(BUILD)/bsearch_probe \
            $(BUILD)/atoi_probe $(BUILD)/errno_probe $(BUILD)/strtol_probe \
            $(BUILD)/strtoul_probe $(BUILD)/strtod_probe \
            $(BUILD)/allocator_probe $(BUILD)/calloc_probe \
            $(BUILD)/realloc_probe $(BUILD)/getenv_probe $(BUILD)/stdio_probe \
            $(BUILD)/file_stream_probe $(BUILD)/block_io_probe $(BUILD)/scan_probe
HOST_TESTS := $(BUILD)/memory_differential $(BUILD)/string_differential \
              $(BUILD)/strtok_differential $(BUILD)/bsearch_differential \
              $(BUILD)/atoi_differential $(BUILD)/strtol_differential \
              $(BUILD)/strtoul_differential $(BUILD)/strtod_differential \
              $(BUILD)/allocator_failure_test $(BUILD)/stdio_write_test \
              $(BUILD)/stdio_block_test $(BUILD)/stdio_scan_test

.PHONY: all clean test inspect

all: $(PROGRAMS) $(HOST_TESTS)

$(BUILD):
	mkdir -p $(BUILD)

$(CRT0): src/crt/crt0.S | $(BUILD)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD)/start.o: src/crt/start.c include/stdlib.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/termination.o: src/crt/termination.c include/stdlib.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/syscall.o: src/syscall/syscall.S | $(BUILD)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD)/memory.o: src/string/memory.c include/string.h include/stddef.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/string.o: src/string/string.c include/string.h include/stddef.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/strerror.o: src/string/strerror.c include/string.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/ctype.o: src/ctype/ctype.c include/ctype.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/atoi.o: src/stdlib/atoi.c include/stdlib.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/strtol.o: src/stdlib/strtol.c include/stdlib.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/strtoul.o: src/stdlib/strtoul.c include/stdlib.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/strtod.o: src/stdlib/strtod.c src/internal/float_parse.h include/stdlib.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/bsearch.o: src/stdlib/bsearch.c include/stdlib.h include/stddef.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/allocator.o: src/stdlib/allocator.c include/stdlib.h include/stddef.h include/errno.h include/mini/syscall.h include/string.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/calloc.o: src/stdlib/calloc.c include/stdlib.h include/stddef.h include/errno.h include/string.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/getenv.o: src/stdlib/getenv.c include/stdlib.h include/stddef.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/stdio.o: src/stdio/stdio.c src/stdio/stdio_internal.h include/stdio.h include/stddef.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/format.o: src/stdio/format.c src/stdio/stdio_internal.h include/stdio.h include/stddef.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/format_entry.o: src/stdio/format_entry.S | $(BUILD)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD)/scan.o: src/stdio/scan.c src/stdio/stdio_internal.h src/internal/float_parse.h include/stdio.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/float_parse.o: src/internal/float_parse.c src/internal/float_parse.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/scan_entry.o: src/stdio/scan_entry.S | $(BUILD)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD)/file_stream.o: src/stdio/file.c src/stdio/stdio_internal.h include/stdio.h include/stdlib.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/block_io.o: src/stdio/block.c src/stdio/stdio_internal.h include/stdio.h include/stddef.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/position.o: src/stdio/position.c src/stdio/stdio_internal.h include/stdio.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/errno.o: src/errno/errno.c include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(LIBC): $(LIB_OBJS)
	$(AR) rcs $@ $^

$(BUILD)/%.o: examples/%.c include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/runtime_probe.o: tests/runtime_probe.c include/mini/syscall.h include/stdlib.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/syscall_probe.o: tests/syscall_probe.c include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/memory_probe.o: tests/memory_probe.c include/mini/syscall.h include/string.h include/stddef.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/memory_diff_impl.o: src/string/memory.c include/string.h include/stddef.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(MEMORY_RENAMES) -c $< -o $@

$(BUILD)/memory_differential.o: tests/memory_differential.c | $(BUILD)
	$(CC) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/string_probe.o: tests/string_probe.c include/mini/syscall.h include/string.h include/stddef.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/strtok_probe.o: tests/strtok_probe.c include/mini/syscall.h include/string.h include/stddef.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/strerror_probe.o: tests/strerror_probe.c include/mini/syscall.h include/string.h include/stddef.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/ctype_probe.o: tests/ctype_probe.c include/mini/syscall.h include/ctype.h include/stdio.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/bsearch_probe.o: tests/bsearch_probe.c include/mini/syscall.h include/stdlib.h include/stddef.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/bsearch_diff_impl.o: src/stdlib/bsearch.c include/stdlib.h include/stddef.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(BSEARCH_RENAMES) -c $< -o $@

$(BUILD)/bsearch_differential.o: tests/bsearch_differential.c | $(BUILD)
	$(CC) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/string_diff_impl.o: src/string/string.c include/string.h include/stddef.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(STRING_RENAMES) -c $< -o $@

$(BUILD)/string_differential.o: tests/string_differential.c | $(BUILD)
	$(CC) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/strtok_differential.o: tests/strtok_differential.c | $(BUILD)
	$(CC) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/atoi_probe.o: tests/atoi_probe.c include/mini/syscall.h include/stdlib.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/atoi_diff_impl.o: src/stdlib/atoi.c include/stdlib.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(ATOI_RENAMES) -c $< -o $@

$(BUILD)/atoi_differential.o: tests/atoi_differential.c | $(BUILD)
	$(CC) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/errno_probe.o: tests/errno_probe.c include/mini/syscall.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/strtol_probe.o: tests/strtol_probe.c include/mini/syscall.h include/stdlib.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/strtol_diff_impl.o: src/stdlib/strtol.c include/stdlib.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(STRTOL_RENAMES) -c $< -o $@

$(BUILD)/strtol_differential.o: tests/strtol_differential.c | $(BUILD)
	$(CC) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/strtoul_probe.o: tests/strtoul_probe.c include/mini/syscall.h include/stdlib.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/strtoul_diff_impl.o: src/stdlib/strtoul.c include/stdlib.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(STRTOUL_RENAMES) -c $< -o $@

$(BUILD)/strtoul_differential.o: tests/strtoul_differential.c | $(BUILD)
	$(CC) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/strtod_probe.o: tests/strtod_probe.c include/mini/syscall.h include/stdlib.h include/stdio.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/float_parse_test_impl.o: src/internal/float_parse.c src/internal/float_parse.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/strtod_diff_impl.o: src/stdlib/strtod.c src/internal/float_parse.h include/stdlib.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(STRTOD_RENAMES) -c $< -o $@

$(BUILD)/strtod_differential.o: tests/strtod_differential.c | $(BUILD)
	$(CC) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/allocator_probe.o: tests/allocator_probe.c include/mini/syscall.h include/stdlib.h include/stddef.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/calloc_probe.o: tests/calloc_probe.c include/mini/syscall.h include/stdlib.h include/stddef.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/realloc_probe.o: tests/realloc_probe.c include/mini/syscall.h include/stdlib.h include/stddef.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/getenv_probe.o: tests/getenv_probe.c include/mini/syscall.h include/stdlib.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/stdio_probe.o: tests/stdio_probe.c include/mini/syscall.h include/stdio.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/file_stream_probe.o: tests/file_stream_probe.c include/mini/syscall.h include/stdio.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/block_io_probe.o: tests/block_io_probe.c include/mini/syscall.h include/stdio.h include/stddef.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/scan_probe.o: tests/scan_probe.c include/mini/syscall.h include/stdio.h include/string.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/stdio_test_impl.o: src/stdio/stdio.c src/stdio/stdio_internal.h include/stdio.h include/stddef.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(STDIO_RENAMES) -c $< -o $@

$(BUILD)/file_stream_test_impl.o: src/stdio/file.c src/stdio/stdio_internal.h include/stdio.h include/stdlib.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(FILE_RENAMES) -c $< -o $@

$(BUILD)/block_io_test_impl.o: src/stdio/block.c src/stdio/stdio_internal.h include/stdio.h include/stddef.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(BLOCK_RENAMES) -c $< -o $@

$(BUILD)/position_test_impl.o: src/stdio/position.c src/stdio/stdio_internal.h include/stdio.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(POSITION_RENAMES) -c $< -o $@

$(BUILD)/scan_test_impl.o: src/stdio/scan.c src/stdio/stdio_internal.h src/internal/float_parse.h include/stdio.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/stdio_write_test.o: tests/stdio_write_test.c include/stdio.h include/stddef.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/stdio_block_test.o: tests/stdio_block_test.c include/stdio.h include/stddef.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/stdio_scan_test.o: tests/stdio_scan_test.c include/stdio.h include/stddef.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/allocator_test_impl.o: src/stdlib/allocator.c include/stdlib.h include/stddef.h include/errno.h include/mini/syscall.h include/string.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(ALLOCATOR_RENAMES) -c $< -o $@

$(BUILD)/allocator_failure_test.o: tests/allocator_failure_test.c | $(BUILD)
	$(CC) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/hello: $(BUILD)/hello.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/hello.o $(CRT0) $(LIBC)

$(BUILD)/runtime_probe: $(BUILD)/runtime_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/runtime_probe.o $(CRT0) $(LIBC)

$(BUILD)/syscall_probe: $(BUILD)/syscall_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/syscall_probe.o $(CRT0) $(LIBC)

$(BUILD)/memory_probe: $(BUILD)/memory_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/memory_probe.o $(CRT0) $(LIBC)

$(BUILD)/string_probe: $(BUILD)/string_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/string_probe.o $(CRT0) $(LIBC)

$(BUILD)/strtok_probe: $(BUILD)/strtok_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/strtok_probe.o $(CRT0) $(LIBC)

$(BUILD)/strerror_probe: $(BUILD)/strerror_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/strerror_probe.o $(CRT0) $(LIBC)

$(BUILD)/ctype_probe: $(BUILD)/ctype_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/ctype_probe.o $(CRT0) $(LIBC)

$(BUILD)/bsearch_probe: $(BUILD)/bsearch_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/bsearch_probe.o $(CRT0) $(LIBC)

$(BUILD)/atoi_probe: $(BUILD)/atoi_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/atoi_probe.o $(CRT0) $(LIBC)

$(BUILD)/errno_probe: $(BUILD)/errno_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/errno_probe.o $(CRT0) $(LIBC)

$(BUILD)/strtol_probe: $(BUILD)/strtol_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/strtol_probe.o $(CRT0) $(LIBC)

$(BUILD)/strtoul_probe: $(BUILD)/strtoul_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/strtoul_probe.o $(CRT0) $(LIBC)

$(BUILD)/strtod_probe: $(BUILD)/strtod_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/strtod_probe.o $(CRT0) $(LIBC)

$(BUILD)/allocator_probe: $(BUILD)/allocator_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/allocator_probe.o $(CRT0) $(LIBC)

$(BUILD)/calloc_probe: $(BUILD)/calloc_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/calloc_probe.o $(CRT0) $(LIBC)

$(BUILD)/realloc_probe: $(BUILD)/realloc_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/realloc_probe.o $(CRT0) $(LIBC)

$(BUILD)/getenv_probe: $(BUILD)/getenv_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/getenv_probe.o $(CRT0) $(LIBC)

$(BUILD)/stdio_probe: $(BUILD)/stdio_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/stdio_probe.o $(CRT0) $(LIBC)

$(BUILD)/file_stream_probe: $(BUILD)/file_stream_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/file_stream_probe.o $(CRT0) $(LIBC)

$(BUILD)/block_io_probe: $(BUILD)/block_io_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/block_io_probe.o $(CRT0) $(LIBC)

$(BUILD)/scan_probe: $(BUILD)/scan_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/scan_probe.o $(CRT0) $(LIBC)

$(BUILD)/memory_differential: $(BUILD)/memory_differential.o $(BUILD)/memory_diff_impl.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^

$(BUILD)/string_differential: $(BUILD)/string_differential.o $(BUILD)/string_diff_impl.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^

$(BUILD)/strtok_differential: $(BUILD)/strtok_differential.o $(BUILD)/string_diff_impl.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^

$(BUILD)/bsearch_differential: $(BUILD)/bsearch_differential.o $(BUILD)/bsearch_diff_impl.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^

$(BUILD)/atoi_differential: $(BUILD)/atoi_differential.o $(BUILD)/atoi_diff_impl.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^

$(BUILD)/strtol_differential: $(BUILD)/strtol_differential.o $(BUILD)/strtol_diff_impl.o $(BUILD)/errno.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^

$(BUILD)/strtoul_differential: $(BUILD)/strtoul_differential.o $(BUILD)/strtoul_diff_impl.o $(BUILD)/errno.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^

$(BUILD)/strtod_differential: $(BUILD)/strtod_differential.o $(BUILD)/strtod_diff_impl.o $(BUILD)/float_parse_test_impl.o $(BUILD)/errno.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^

$(BUILD)/allocator_failure_test: $(BUILD)/allocator_failure_test.o $(BUILD)/allocator_test_impl.o $(BUILD)/errno.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^

$(BUILD)/stdio_write_test: $(BUILD)/stdio_write_test.o $(BUILD)/stdio_test_impl.o $(BUILD)/file_stream_test_impl.o $(BUILD)/errno.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^

$(BUILD)/stdio_block_test: $(BUILD)/stdio_block_test.o $(BUILD)/stdio_test_impl.o $(BUILD)/block_io_test_impl.o $(BUILD)/position_test_impl.o $(BUILD)/errno.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^

$(BUILD)/stdio_scan_test: $(BUILD)/stdio_scan_test.o $(BUILD)/stdio_test_impl.o $(BUILD)/scan_test_impl.o $(BUILD)/float_parse_test_impl.o $(BUILD)/scan_entry.o $(BUILD)/errno.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^

test: all
	./tests/run.sh

inspect: all
	./tests/verify-no-host-libc.sh $(PROGRAMS)

clean:
	rm -rf $(BUILD)
