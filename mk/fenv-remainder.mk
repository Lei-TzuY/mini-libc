$(BUILD)/math_remainder.o: include/fenv.h
$(BUILD)/math_remainder_diff_impl.o: include/fenv.h
$(BUILD)/math_remainder_diff_impl.o: CPPFLAGS += $(FENV_RENAMES)
$(BUILD)/remainder_differential: $(BUILD)/fenv_test_impl.o $(BUILD)/fenv_asm.o

all: $(BUILD)/fenv_remainder_probe $(BUILD)/fenv_remainder_interop
inspect: fenv_remainder_inspect
test: fenv_remainder_test_run

.PHONY: fenv_remainder_inspect fenv_remainder_test_run

$(BUILD)/fenv_remainder_probe.o: tests/fenv_remainder_probe.c include/fenv.h include/math.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/fenv_remainder_probe: $(BUILD)/fenv_remainder_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/fenv_remainder_probe.o $(CRT0) $(LIBC)

$(BUILD)/fenv_remainder_interop.o: tests/fenv_remainder_interop.c | $(BUILD)
	$(CC) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/fenv_remainder_interop: $(BUILD)/fenv_remainder_interop.o \
                                 $(BUILD)/math_remainder_diff_impl.o \
                                 $(BUILD)/math_decompose_diff_impl.o \
                                 $(BUILD)/math_classify.o \
                                 $(BUILD)/fenv_test_impl.o \
                                 $(BUILD)/fenv_asm.o $(BUILD)/errno.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^ -lm

fenv_remainder_test_run: $(BUILD)/fenv_remainder_probe $(BUILD)/fenv_remainder_interop
	@output=$$($(BUILD)/fenv_remainder_probe); status=$$?; \
		test $$status -eq 0 -a "$$output" = "fenv-remainder-ok" || { \
			echo "unexpected fenv remainder probe: status=$$status output='$$output'" >&2; exit 1; }
	@output=$$($(BUILD)/fenv_remainder_interop); status=$$?; \
		test $$status -eq 0 -a "$$output" = "fenv-remainder-interop-ok" || { \
			echo "unexpected fenv remainder interop: status=$$status output='$$output'" >&2; exit 1; }

fenv_remainder_inspect: $(BUILD)/fenv_remainder_probe
	./tests/verify-no-host-libc.sh $(BUILD)/fenv_remainder_probe
