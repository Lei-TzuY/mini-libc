all: $(BUILD)/fenv_closure_probe $(BUILD)/fenv_closure_interop
inspect: fenv_closure_inspect
test: fenv_closure_test_run

.PHONY: fenv_closure_inspect fenv_closure_test_run

$(BUILD)/fenv_closure_probe.o: tests/fenv_closure_probe.c include/fenv.h include/math.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/fenv_closure_probe: $(BUILD)/fenv_closure_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/fenv_closure_probe.o $(CRT0) $(LIBC)

$(BUILD)/fenv_closure_interop.o: tests/fenv_closure_interop.c | $(BUILD)
	$(CC) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/fenv_closure_interop: $(BUILD)/fenv_closure_interop.o \
                               $(BUILD)/math_diff_impl.o \
                               $(BUILD)/math_decompose_diff_impl.o \
                               $(BUILD)/math_classify.o \
                               $(BUILD)/math_sqrt.o $(BUILD)/errno.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^ -lm

fenv_closure_test_run: $(BUILD)/fenv_closure_probe $(BUILD)/fenv_closure_interop
	@output=$$($(BUILD)/fenv_closure_probe); status=$$?; \
		test $$status -eq 0 -a "$$output" = "fenv-closure-ok" || { \
			echo "unexpected fenv closure probe: status=$$status output='$$output'" >&2; exit 1; }
	@output=$$($(BUILD)/fenv_closure_interop); status=$$?; \
		test $$status -eq 0 -a "$$output" = "fenv-closure-interop-ok" || { \
			echo "unexpected fenv closure interop: status=$$status output='$$output'" >&2; exit 1; }

fenv_closure_inspect: $(BUILD)/fenv_closure_probe
	./tests/verify-no-host-libc.sh $(BUILD)/fenv_closure_probe
