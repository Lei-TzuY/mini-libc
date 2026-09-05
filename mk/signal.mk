SIGNAL_RENAMES := -Dmini_sys_rt_sigaction=mini_test_rt_sigaction \
                  -Dmini_sys_getpid=mini_test_getpid \
                  -Dmini_sys_gettid=mini_test_gettid \
                  -Dmini_sys_tgkill=mini_test_tgkill \
                  -D_Exit=mini_test__Exit

$(LIBC): $(BUILD)/signal.o
all: $(BUILD)/signal_probe $(BUILD)/signal_test
inspect: signal_inspect

test: signal_test_run

.PHONY: signal_inspect signal_test_run

$(BUILD)/signal.o: src/signal/signal.c include/signal.h include/stdlib.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/signal_probe.o: tests/signal_probe.c include/signal.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/signal_probe: $(BUILD)/signal_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/signal_probe.o $(CRT0) $(LIBC)

$(BUILD)/signal_test_impl.o: src/signal/signal.c include/signal.h include/stdlib.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SIGNAL_RENAMES) -c $< -o $@

$(BUILD)/signal_test.o: tests/signal_test.c include/signal.h include/stdlib.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/signal_test: $(BUILD)/signal_test.o $(BUILD)/signal_test_impl.o $(BUILD)/errno.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^

signal_test_run: $(BUILD)/signal_probe $(BUILD)/signal_test $(BUILD)/runtime_probe
	sh ./tests/run-signal.sh
	sh ./tests/run-termination.sh

signal_inspect: $(BUILD)/signal_probe
	./tests/verify-no-host-libc.sh $(BUILD)/signal_probe
