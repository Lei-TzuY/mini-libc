$(BUILD)/math_hyperbolic.o: include/fenv.h
$(BUILD)/math_hyperbolic_diff_impl.o: include/fenv.h

all: $(BUILD)/fenv_hyperbolic_probe $(BUILD)/fenv_hyperbolic_interop
inspect: fenv_hyperbolic_inspect
test: fenv_hyperbolic_test_run

.PHONY: fenv_hyperbolic_inspect fenv_hyperbolic_test_run

$(BUILD)/fenv_hyperbolic_probe.o: tests/fenv_hyperbolic_probe.c include/fenv.h include/math.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/fenv_hyperbolic_probe: $(BUILD)/fenv_hyperbolic_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/fenv_hyperbolic_probe.o $(CRT0) $(LIBC)

$(BUILD)/fenv_hyperbolic_hyperbolic_test_impl.o: src/math/hyperbolic.c include/math.h include/fenv.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(MATH_RENAMES) $(FENV_RENAMES) -c $< -o $@

$(BUILD)/fenv_hyperbolic_interop.o: tests/fenv_hyperbolic_interop.c | $(BUILD)
	$(CC) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/fenv_hyperbolic_interop: $(BUILD)/fenv_hyperbolic_interop.o \
                                  $(BUILD)/fenv_hyperbolic_hyperbolic_test_impl.o \
                                  $(BUILD)/fenv_domain_math_test_impl.o \
                                  $(BUILD)/fenv_explog_test_impl.o \
                                  $(BUILD)/math_decompose_diff_impl.o \
                                  $(BUILD)/math_sqrt.o \
                                  $(BUILD)/fenv_test_impl.o \
                                  $(BUILD)/fenv_asm.o $(BUILD)/errno.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^ -lm

fenv_hyperbolic_test_run: $(BUILD)/fenv_hyperbolic_probe $(BUILD)/fenv_hyperbolic_interop
	@output=$$($(BUILD)/fenv_hyperbolic_probe); status=$$?; \
		test $$status -eq 0 -a "$$output" = "fenv-hyperbolic-ok" || { \
			echo "unexpected fenv hyperbolic probe: status=$$status output='$$output'" >&2; exit 1; }
	@test "$$($(BUILD)/fenv_hyperbolic_interop)" = "fenv-hyperbolic-interop-ok" || { echo "unexpected fenv hyperbolic interop output" >&2; exit 1; }

fenv_hyperbolic_inspect: $(BUILD)/fenv_hyperbolic_probe
	./tests/verify-no-host-libc.sh $(BUILD)/fenv_hyperbolic_probe
