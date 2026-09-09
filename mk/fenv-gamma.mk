$(BUILD)/math_gamma.o: include/fenv.h
$(BUILD)/math_gamma_diff_impl.o: include/fenv.h

all: $(BUILD)/fenv_gamma_probe $(BUILD)/fenv_gamma_interop
inspect: fenv_gamma_inspect
test: fenv_gamma_test_run

.PHONY: fenv_gamma_inspect fenv_gamma_test_run

$(BUILD)/fenv_gamma_probe.o: tests/fenv_gamma_probe.c include/fenv.h include/math.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/fenv_gamma_probe: $(BUILD)/fenv_gamma_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/fenv_gamma_probe.o $(CRT0) $(LIBC)

$(BUILD)/fenv_gamma_gamma_test_impl.o: src/math/gamma.c include/math.h include/fenv.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(GAMMA_RENAMES) $(FENV_RENAMES) -c $< -o $@

$(BUILD)/fenv_gamma_interop.o: tests/fenv_gamma_interop.c | $(BUILD)
	$(CC) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/fenv_gamma_interop: $(BUILD)/fenv_gamma_interop.o \
                             $(BUILD)/fenv_gamma_gamma_test_impl.o \
                             $(BUILD)/math_trig_diff_impl.o \
                             $(BUILD)/fenv_explog_test_impl.o \
                             $(BUILD)/math_decompose_diff_impl.o \
                             $(BUILD)/fenv_test_impl.o \
                             $(BUILD)/fenv_asm.o $(BUILD)/errno.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^ -lm

fenv_gamma_test_run: $(BUILD)/fenv_gamma_probe $(BUILD)/fenv_gamma_interop
	@output=$$($(BUILD)/fenv_gamma_probe); status=$$?; \
		test $$status -eq 0 -a "$$output" = "fenv-gamma-ok" || { \
			echo "unexpected fenv gamma probe: status=$$status output='$$output'" >&2; exit 1; }
	@test "$$($(BUILD)/fenv_gamma_interop)" = "fenv-gamma-interop-ok" || { echo "unexpected fenv gamma interop output" >&2; exit 1; }

fenv_gamma_inspect: $(BUILD)/fenv_gamma_probe
	./tests/verify-no-host-libc.sh $(BUILD)/fenv_gamma_probe
