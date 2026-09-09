$(BUILD)/math_special.o: include/fenv.h
$(BUILD)/math_special_diff_impl.o: include/fenv.h

all: $(BUILD)/fenv_special_probe $(BUILD)/fenv_special_interop
inspect: fenv_special_inspect
test: fenv_special_test_run

.PHONY: fenv_special_inspect fenv_special_test_run

$(BUILD)/fenv_special_probe.o: tests/fenv_special_probe.c include/fenv.h include/math.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/fenv_special_probe: $(BUILD)/fenv_special_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/fenv_special_probe.o $(CRT0) $(LIBC)

$(BUILD)/fenv_special_special_test_impl.o: src/math/special.c include/math.h include/fenv.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SPECIAL_RENAMES) $(FENV_RENAMES) -c $< -o $@

$(BUILD)/fenv_special_interop.o: tests/fenv_special_interop.c | $(BUILD)
	$(CC) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/fenv_special_interop: $(BUILD)/fenv_special_interop.o \
                               $(BUILD)/fenv_special_special_test_impl.o \
                               $(BUILD)/fenv_explog_test_impl.o \
                               $(BUILD)/math_decompose_diff_impl.o \
                               $(BUILD)/fenv_test_impl.o \
                               $(BUILD)/fenv_asm.o $(BUILD)/errno.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^ -lm

fenv_special_test_run: $(BUILD)/fenv_special_probe $(BUILD)/fenv_special_interop
	@output=$$($(BUILD)/fenv_special_probe); status=$$?; \
		test $$status -eq 0 -a "$$output" = "fenv-special-ok" || { \
			echo "unexpected fenv special probe: status=$$status output='$$output'" >&2; exit 1; }
	@output=$$($(BUILD)/fenv_special_interop); status=$$?; \
		test $$status -eq 0 -a "$$output" = "fenv-special-interop-ok" || { \
			echo "unexpected fenv special interop: status=$$status output='$$output'" >&2; exit 1; }

fenv_special_inspect: $(BUILD)/fenv_special_probe
	./tests/verify-no-host-libc.sh $(BUILD)/fenv_special_probe
