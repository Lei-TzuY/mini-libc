$(BUILD)/math_trig.o: include/fenv.h
$(BUILD)/math_trig_diff_impl.o: include/fenv.h

all: $(BUILD)/fenv_trig_probe $(BUILD)/fenv_trig_interop
inspect: fenv_trig_inspect
test: fenv_trig_test_run

.PHONY: fenv_trig_inspect fenv_trig_test_run

$(BUILD)/fenv_trig_probe.o: tests/fenv_trig_probe.c include/fenv.h include/math.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/fenv_trig_probe: $(BUILD)/fenv_trig_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/fenv_trig_probe.o $(CRT0) $(LIBC)

$(BUILD)/fenv_trig_trig_test_impl.o: src/math/trig.c include/math.h include/fenv.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(MATH_RENAMES) $(FENV_RENAMES) -c $< -o $@

$(BUILD)/fenv_trig_interop.o: tests/fenv_trig_interop.c | $(BUILD)
	$(CC) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/fenv_trig_interop: $(BUILD)/fenv_trig_interop.o \
                           $(BUILD)/fenv_trig_trig_test_impl.o \
                           $(BUILD)/fenv_test_impl.o \
                           $(BUILD)/fenv_asm.o $(BUILD)/errno.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^ -lm

fenv_trig_test_run: $(BUILD)/fenv_trig_probe $(BUILD)/fenv_trig_interop
	@output=$$($(BUILD)/fenv_trig_probe); status=$$?; \
		test $$status -eq 0 -a "$$output" = "fenv-trig-ok" || { \
			echo "unexpected fenv trig probe: status=$$status output='$$output'" >&2; exit 1; }
	@test "$$($(BUILD)/fenv_trig_interop)" = "fenv-trig-interop-ok" || { echo "unexpected fenv trig interop output" >&2; exit 1; }

fenv_trig_inspect: $(BUILD)/fenv_trig_probe
	./tests/verify-no-host-libc.sh $(BUILD)/fenv_trig_probe
