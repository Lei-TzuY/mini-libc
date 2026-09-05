$(LIBC): $(BUILD)/atomic.o $(BUILD)/thread_runtime.o $(BUILD)/thread.o $(BUILD)/thread_entry.o
all: $(BUILD)/thread_probe $(BUILD)/thread_exit_group_probe
inspect: thread_inspect
test: thread_test_run

.PHONY: thread_inspect thread_test_run

$(BUILD)/atomic.o: src/internal/atomic.S | $(BUILD)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD)/thread_runtime.o: src/thread/runtime.c src/internal/thread_runtime.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/thread.o: src/thread/thread.c src/internal/thread_runtime.h include/threads.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/thread_entry.o: src/thread/thread_entry.S | $(BUILD)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD)/thread_probe.o: tests/thread_probe.c include/threads.h include/stdlib.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/thread_probe: $(BUILD)/thread_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/thread_probe.o $(CRT0) $(LIBC)

$(BUILD)/thread_exit_group_probe.o: tests/thread_exit_group_probe.c include/threads.h include/stdlib.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/thread_exit_group_probe: $(BUILD)/thread_exit_group_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/thread_exit_group_probe.o $(CRT0) $(LIBC)

thread_test_run: $(BUILD)/thread_probe $(BUILD)/thread_exit_group_probe
	@test "$$($(BUILD)/thread_probe)" = "threads-ok"
	@output="$$($(BUILD)/thread_exit_group_probe)"; status=$$?; \
		test "$$status" -eq 37 && test -z "$$output"

thread_inspect: $(BUILD)/thread_probe $(BUILD)/thread_exit_group_probe
	./tests/verify-no-host-libc.sh $(BUILD)/thread_probe $(BUILD)/thread_exit_group_probe
