$(BUILD)/math.o: include/fenv.h
$(BUILD)/math_diff_impl.o: include/fenv.h
$(BUILD)/math_inverse_trig.o: include/fenv.h
$(BUILD)/math_inverse_trig_diff_impl.o: include/fenv.h
$(BUILD)/math_hyperbolic.o: include/fenv.h
$(BUILD)/math_hyperbolic_diff_impl.o: include/fenv.h

all: $(BUILD)/fenv_domain_probe $(BUILD)/fenv_domain_interop
inspect: fenv_domain_inspect
test: fenv_domain_test_run

.PHONY: fenv_domain_inspect fenv_domain_test_run

$(BUILD)/fenv_domain_probe.o: tests/fenv_domain_probe.c include/fenv.h include/math.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/fenv_domain_probe: $(BUILD)/fenv_domain_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/fenv_domain_probe.o $(CRT0) $(LIBC)

$(BUILD)/fenv_domain_math_test_impl.o: src/math/math.c include/math.h include/fenv.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(MATH_RENAMES) $(FENV_RENAMES) -c $< -o $@

$(BUILD)/fenv_domain_inverse_trig_test_impl.o: src/math/inverse_trig.c include/math.h include/fenv.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(MATH_RENAMES) $(FENV_RENAMES) -c $< -o $@

$(BUILD)/fenv_domain_hyperbolic_test_impl.o: src/math/hyperbolic.c include/math.h include/fenv.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(MATH_RENAMES) $(FENV_RENAMES) -c $< -o $@

$(BUILD)/fenv_domain_interop.o: tests/fenv_domain_interop.c | $(BUILD)
	$(CC) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/fenv_domain_interop: $(BUILD)/fenv_domain_interop.o \
                              $(BUILD)/fenv_domain_math_test_impl.o \
                              $(BUILD)/fenv_domain_inverse_trig_test_impl.o \
                              $(BUILD)/fenv_domain_hyperbolic_test_impl.o \
                              $(BUILD)/fenv_explog_test_impl.o \
                              $(BUILD)/math_decompose_diff_impl.o \
                              $(BUILD)/math_sqrt.o \
                              $(BUILD)/fenv_test_impl.o \
                              $(BUILD)/fenv_asm.o $(BUILD)/errno.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^ -lm

fenv_domain_test_run: $(BUILD)/fenv_domain_probe $(BUILD)/fenv_domain_interop
	@output=$$($(BUILD)/fenv_domain_probe); status=$$?; \
		test $$status -eq 0 -a "$$output" = "fenv-domain-ok" || { \
			echo "unexpected fenv domain probe: status=$$status output='$$output'" >&2; exit 1; }
	@test "$$($(BUILD)/fenv_domain_interop)" = "fenv-domain-interop-ok" || { echo "unexpected fenv domain interop output" >&2; exit 1; }

fenv_domain_inspect: $(BUILD)/fenv_domain_probe
	./tests/verify-no-host-libc.sh $(BUILD)/fenv_domain_probe
