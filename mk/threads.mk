CONDITION_RENAMES := -Dmini_sys_futex=mini_test_futex \
                     -Dmtx_lock=mini_test_mtx_lock \
                     -Dmtx_unlock=mini_test_mtx_unlock
SLEEP_RENAMES := -Dmini_sys_nanosleep=mini_test_nanosleep

$(LIBC): $(BUILD)/atomic.o $(BUILD)/condition.o $(BUILD)/mutex.o $(BUILD)/sleep.o $(BUILD)/once.o $(BUILD)/tss.o $(BUILD)/thread_runtime.o $(BUILD)/lifecycle.o $(BUILD)/thread_entry.o
all: $(BUILD)/thread_probe $(BUILD)/thread_exit_group_probe $(BUILD)/condition_probe $(BUILD)/mutex_probe $(BUILD)/once_tss_probe $(BUILD)/condition_test $(BUILD)/mutex_test $(BUILD)/once_tss_test
inspect: thread_inspect
test: thread_test_run

.PHONY: thread_inspect thread_test_run

$(BUILD)/atomic.o: src/internal/atomic.S | $(BUILD)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD)/condition.o: src/thread/condition.c include/threads.h include/time.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/mutex.o: src/thread/mutex.c src/internal/thread_runtime.h include/threads.h include/time.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/sleep.o: src/thread/sleep.c include/threads.h include/time.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/once.o: src/thread/once.c include/threads.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/tss.o: src/thread/tss.c src/internal/thread_runtime.h include/threads.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/thread_runtime.o: src/thread/runtime.c src/internal/thread_runtime.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/lifecycle.o: src/thread/lifecycle.c src/internal/thread_runtime.h include/threads.h include/mini/syscall.h | $(BUILD)
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

$(BUILD)/condition_probe.o: tests/condition_probe.c include/threads.h include/time.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/condition_probe: $(BUILD)/condition_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/condition_probe.o $(CRT0) $(LIBC)

$(BUILD)/mutex_probe.o: tests/mutex_probe.c include/threads.h include/time.h include/errno.h include/stdio.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/mutex_probe: $(BUILD)/mutex_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/mutex_probe.o $(CRT0) $(LIBC)

$(BUILD)/once_tss_probe.o: tests/once_tss_probe.c include/threads.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/once_tss_probe: $(BUILD)/once_tss_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/once_tss_probe.o $(CRT0) $(LIBC)

$(BUILD)/condition_test_impl.o: src/thread/condition.c include/threads.h include/time.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CONDITION_RENAMES) -c $< -o $@

$(BUILD)/sleep_test_impl.o: src/thread/sleep.c include/threads.h include/time.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SLEEP_RENAMES) -c $< -o $@

$(BUILD)/condition_test.o: tests/condition_test.c include/threads.h include/time.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/condition_test: $(BUILD)/condition_test.o $(BUILD)/condition_test_impl.o $(BUILD)/sleep_test_impl.o $(BUILD)/errno.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^

$(BUILD)/mutex_test_impl.o: src/thread/mutex.c src/internal/thread_runtime.h include/threads.h include/time.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/mutex_test.o: tests/mutex_test.c include/threads.h include/time.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/mutex_test: $(BUILD)/mutex_test.o $(BUILD)/mutex_test_impl.o $(BUILD)/atomic.o $(BUILD)/errno.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^

$(BUILD)/once_test_impl.o: src/thread/once.c include/threads.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/tss_test_impl.o: src/thread/tss.c src/internal/thread_runtime.h include/threads.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/once_tss_test.o: tests/once_tss_test.c include/threads.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/once_tss_test: $(BUILD)/once_tss_test.o $(BUILD)/once_test_impl.o $(BUILD)/tss_test_impl.o $(BUILD)/errno.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^

thread_test_run: $(BUILD)/thread_probe $(BUILD)/thread_exit_group_probe $(BUILD)/condition_probe $(BUILD)/mutex_probe $(BUILD)/once_tss_probe $(BUILD)/condition_test $(BUILD)/mutex_test $(BUILD)/once_tss_test
	@test "$$($(BUILD)/thread_probe)" = "threads-ok"
	@output="$$($(BUILD)/thread_exit_group_probe)"; status=$$?; \
		test "$$status" -eq 37 && test -z "$$output"
	@test "$$($(BUILD)/condition_probe)" = "conditions-ok"
	@test "$$($(BUILD)/mutex_probe)" = "mutex-types-ok"
	@test "$$($(BUILD)/once_tss_probe)" = "once-tss-ok"
	@$(BUILD)/condition_test
	@$(BUILD)/mutex_test
	@$(BUILD)/once_tss_test

thread_inspect: $(BUILD)/thread_probe $(BUILD)/thread_exit_group_probe $(BUILD)/condition_probe $(BUILD)/mutex_probe $(BUILD)/once_tss_probe
	./tests/verify-no-host-libc.sh $(BUILD)/thread_probe $(BUILD)/thread_exit_group_probe $(BUILD)/condition_probe $(BUILD)/mutex_probe $(BUILD)/once_tss_probe

include mk/stdio-sync.mk
