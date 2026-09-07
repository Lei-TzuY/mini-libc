STDIO_LOCK_RENAMES := -Dmini_sys_futex=mini_test_futex

$(LIBC): $(BUILD)/stdio_lock.o $(BUILD)/file_sync.o $(BUILD)/position_sync.o $(BUILD)/format_sync.o $(BUILD)/scan_sync.o
all: $(BUILD)/stdio_thread_probe
 test: stdio_sync_test_run
inspect: stdio_sync_inspect

.PHONY: stdio_sync_test_run stdio_sync_inspect

$(BUILD)/stdio_lock.o: src/stdio/lock.c src/internal/futex_lock.h src/internal/thread_runtime.h include/stdatomic.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/file_sync.o: src/stdio/file_sync.c src/stdio/stdio_internal.h include/stdio.h include/stddef.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/position_sync.o: src/stdio/position_sync.c src/stdio/stdio_internal.h include/stdio.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/format_sync.o: src/stdio/format_sync.c src/stdio/stdio_internal.h include/stdio.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/scan_sync.o: src/stdio/scan_sync.c src/stdio/stdio_internal.h include/stdio.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/stdio_lock_fake.o: tests/stdio_lock_fake.c | $(BUILD)
	$(CC) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/stdio_write_lseek_fake.o: tests/stdio_write_lseek_fake.c | $(BUILD)
	$(CC) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/stdio_lock_test_impl.o: src/stdio/lock.c src/internal/futex_lock.h src/internal/thread_runtime.h include/stdatomic.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(STDIO_LOCK_RENAMES) -c $< -o $@

$(BUILD)/stdio_lock_test.o: tests/stdio_lock_test.c src/internal/thread_runtime.h include/stdatomic.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/stdio_lock_test: $(BUILD)/stdio_lock_test.o $(BUILD)/stdio_lock_test_impl.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^

$(BUILD)/stdio_write_test: $(BUILD)/file_sync.o $(BUILD)/position_test_impl.o \
                          $(BUILD)/stdio_write_lseek_fake.o $(BUILD)/stdio_lock_fake.o
$(BUILD)/stdio_block_test: $(BUILD)/position_sync.o $(BUILD)/stdio_lock_fake.o
$(BUILD)/stdio_scan_test: $(BUILD)/scan_sync.o $(BUILD)/stdio_lock_fake.o

$(BUILD)/stdio_thread_probe.o: tests/stdio_thread_probe.c include/stdio.h include/string.h include/threads.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/stdio_thread_probe: $(BUILD)/stdio_thread_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/stdio_thread_probe.o $(CRT0) $(LIBC)

stdio_sync_test_run: $(BUILD)/stdio_thread_probe $(BUILD)/stdio_lock_test
	@test "$$($(BUILD)/stdio_thread_probe)" = "stdio-thread-ok"
	@$(BUILD)/stdio_lock_test

stdio_sync_inspect: $(BUILD)/stdio_thread_probe
	./tests/verify-no-host-libc.sh $(BUILD)/stdio_thread_probe
