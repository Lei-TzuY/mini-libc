$(LIBC): $(BUILD)/math_scale.o

$(BUILD)/math_scale.o: src/math/scale.c include/math.h include/fenv.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/math_scale_diff_impl.o: src/math/scale.c include/math.h include/fenv.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(MATH_RENAMES) $(FENV_RENAMES) -c $< -o $@

SCALE_DIFF_USERS := $(BUILD)/math_differential \
                    $(BUILD)/remainder_differential \
                    $(BUILD)/explog_differential \
                    $(BUILD)/pow_differential \
                    $(BUILD)/hyperbolic_differential \
                    $(BUILD)/special_differential \
                    $(BUILD)/gamma_differential \
                    $(BUILD)/fenv_explog_interop \
                    $(BUILD)/fenv_pow_interop \
                    $(BUILD)/fenv_remainder_interop \
                    $(BUILD)/fenv_domain_interop \
                    $(BUILD)/fenv_hyperbolic_interop \
                    $(BUILD)/fenv_gamma_interop \
                    $(BUILD)/fenv_special_interop

$(SCALE_DIFF_USERS): $(BUILD)/math_scale_diff_impl.o \
                     $(BUILD)/fenv_test_impl.o $(BUILD)/fenv_asm.o

all: $(BUILD)/fenv_scaling_probe $(BUILD)/fenv_scaling_interop
inspect: fenv_scaling_inspect
test: fenv_scaling_test_run

.PHONY: fenv_scaling_inspect fenv_scaling_test_run

$(BUILD)/fenv_scaling_probe.o: tests/fenv_scaling_probe.c include/fenv.h include/math.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/fenv_scaling_probe: $(BUILD)/fenv_scaling_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/fenv_scaling_probe.o $(CRT0) $(LIBC)

$(BUILD)/fenv_scaling_interop.o: tests/fenv_scaling_interop.c | $(BUILD)
	$(CC) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/fenv_scaling_interop: $(BUILD)/fenv_scaling_interop.o \
                               $(BUILD)/math_scale_diff_impl.o \
                               $(BUILD)/fenv_test_impl.o \
                               $(BUILD)/fenv_asm.o $(BUILD)/errno.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^ -lm

fenv_scaling_test_run: $(BUILD)/fenv_scaling_probe $(BUILD)/fenv_scaling_interop
	@output=$$($(BUILD)/fenv_scaling_probe); status=$$?; \
		test $$status -eq 0 -a "$$output" = "fenv-scaling-ok" || { \
			echo "unexpected fenv scaling probe: status=$$status output='$$output'" >&2; exit 1; }
	@output=$$($(BUILD)/fenv_scaling_interop); status=$$?; \
		test $$status -eq 0 -a "$$output" = "fenv-scaling-interop-ok" || { \
			echo "unexpected fenv scaling interop: status=$$status output='$$output'" >&2; exit 1; }

fenv_scaling_inspect: $(BUILD)/fenv_scaling_probe
	./tests/verify-no-host-libc.sh $(BUILD)/fenv_scaling_probe
