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
                  -Dmemset=mini_test_memset -Dmemcmp=mini_test_memcmp
STRING_RENAMES := -Dstrlen=mini_test_strlen -Dstrcmp=mini_test_strcmp \
                  -Dstrncmp=mini_test_strncmp -Dstrcpy=mini_test_strcpy \
                  -Dstrncpy=mini_test_strncpy -Dstrchr=mini_test_strchr \
                  -Dstrrchr=mini_test_strrchr
ATOI_RENAMES := -Datoi=mini_test_atoi
STRTOL_RENAMES := -Dstrtol=mini_test_strtol
STRTOUL_RENAMES := -Dstrtoul=mini_test_strtoul

BUILD := build
LIBC := $(BUILD)/libc.a
CRT0 := $(BUILD)/crt0.o
LIB_OBJS := $(BUILD)/start.o $(BUILD)/syscall.o $(BUILD)/memory.o $(BUILD)/string.o \
            $(BUILD)/atoi.o $(BUILD)/strtol.o $(BUILD)/strtoul.o $(BUILD)/errno.o
PROGRAMS := $(BUILD)/hello $(BUILD)/runtime_probe $(BUILD)/syscall_probe \
            $(BUILD)/memory_probe $(BUILD)/string_probe $(BUILD)/atoi_probe \
            $(BUILD)/errno_probe $(BUILD)/strtol_probe $(BUILD)/strtoul_probe
HOST_TESTS := $(BUILD)/memory_differential $(BUILD)/string_differential \
              $(BUILD)/atoi_differential $(BUILD)/strtol_differential \
              $(BUILD)/strtoul_differential

.PHONY: all clean test inspect

all: $(PROGRAMS) $(HOST_TESTS)

$(BUILD):
	mkdir -p $(BUILD)

$(CRT0): src/crt/crt0.S | $(BUILD)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD)/start.o: src/crt/start.c include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/syscall.o: src/syscall/syscall.S | $(BUILD)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD)/memory.o: src/string/memory.c include/string.h include/stddef.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/string.o: src/string/string.c include/string.h include/stddef.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/atoi.o: src/stdlib/atoi.c include/stdlib.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/strtol.o: src/stdlib/strtol.c include/stdlib.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/strtoul.o: src/stdlib/strtoul.c include/stdlib.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/errno.o: src/errno/errno.c include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(LIBC): $(LIB_OBJS)
	$(AR) rcs $@ $^

$(BUILD)/%.o: examples/%.c include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/runtime_probe.o: tests/runtime_probe.c include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/syscall_probe.o: tests/syscall_probe.c include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/memory_probe.o: tests/memory_probe.c include/mini/syscall.h include/string.h include/stddef.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/memory_diff_impl.o: src/string/memory.c include/string.h include/stddef.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(MEMORY_RENAMES) -c $< -o $@

$(BUILD)/memory_differential.o: tests/memory_differential.c | $(BUILD)
	$(CC) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/string_probe.o: tests/string_probe.c include/mini/syscall.h include/string.h include/stddef.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/string_diff_impl.o: src/string/string.c include/string.h include/stddef.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(STRING_RENAMES) -c $< -o $@

$(BUILD)/string_differential.o: tests/string_differential.c | $(BUILD)
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

$(BUILD)/atoi_probe: $(BUILD)/atoi_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/atoi_probe.o $(CRT0) $(LIBC)

$(BUILD)/errno_probe: $(BUILD)/errno_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/errno_probe.o $(CRT0) $(LIBC)

$(BUILD)/strtol_probe: $(BUILD)/strtol_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/strtol_probe.o $(CRT0) $(LIBC)

$(BUILD)/strtoul_probe: $(BUILD)/strtoul_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/strtoul_probe.o $(CRT0) $(LIBC)

$(BUILD)/memory_differential: $(BUILD)/memory_differential.o $(BUILD)/memory_diff_impl.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^

$(BUILD)/string_differential: $(BUILD)/string_differential.o $(BUILD)/string_diff_impl.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^

$(BUILD)/atoi_differential: $(BUILD)/atoi_differential.o $(BUILD)/atoi_diff_impl.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^

$(BUILD)/strtol_differential: $(BUILD)/strtol_differential.o $(BUILD)/strtol_diff_impl.o $(BUILD)/errno.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^

$(BUILD)/strtoul_differential: $(BUILD)/strtoul_differential.o $(BUILD)/strtoul_diff_impl.o $(BUILD)/errno.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^

test: all
	./tests/run.sh

inspect: all
	./tests/verify-no-host-libc.sh $(PROGRAMS)

clean:
	rm -rf $(BUILD)
