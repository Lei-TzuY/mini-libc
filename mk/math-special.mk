SPECIAL_RENAMES := $(MATH_RENAMES) \
                   -Derf=mini_test_erf -Derff=mini_test_erff \
                   -Derfc=mini_test_erfc -Derfcf=mini_test_erfcf
GAMMA_RENAMES := $(MATH_RENAMES) \
                 -Dtgamma=mini_test_tgamma -Dtgammaf=mini_test_tgammaf \
                 -Dlgamma=mini_test_lgamma -Dlgammaf=mini_test_lgammaf

$(LIBC): $(BUILD)/math_special.o $(BUILD)/math_gamma.o
all: $(BUILD)/special_probe $(BUILD)/special_differential \
     $(BUILD)/gamma_probe $(BUILD)/gamma_differential
inspect: math_special_inspect
test: math_special_test_run

.PHONY: math_special_inspect math_special_test_run

$(BUILD)/math_special.o: src/math/special.c include/math.h include/fenv.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/math_gamma.o: src/math/gamma.c include/math.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/special_probe.o: tests/special_probe.c include/math.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/special_probe: $(BUILD)/special_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/special_probe.o $(CRT0) $(LIBC)

$(BUILD)/gamma_probe.o: tests/gamma_probe.c include/math.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/gamma_probe: $(BUILD)/gamma_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/gamma_probe.o $(CRT0) $(LIBC)

$(BUILD)/math_special_diff_impl.o: src/math/special.c include/math.h include/fenv.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SPECIAL_RENAMES) $(FENV_RENAMES) -c $< -o $@

$(BUILD)/math_gamma_diff_impl.o: src/math/gamma.c include/math.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(GAMMA_RENAMES) -c $< -o $@

$(BUILD)/special_differential.o: tests/special_differential.c | $(BUILD)
	$(CC) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/special_differential: $(BUILD)/special_differential.o \
                               $(BUILD)/math_special_diff_impl.o \
                               $(BUILD)/math_explog_diff_impl.o \
                               $(BUILD)/math_decompose_diff_impl.o \
                               $(BUILD)/fenv_test_impl.o \
                               $(BUILD)/fenv_asm.o $(BUILD)/errno.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^ -lm

$(BUILD)/gamma_differential.o: tests/gamma_differential.c | $(BUILD)
	$(CC) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/gamma_differential: $(BUILD)/gamma_differential.o \
                             $(BUILD)/math_gamma_diff_impl.o \
                             $(BUILD)/math_trig_diff_impl.o \
                             $(BUILD)/math_explog_diff_impl.o \
                             $(BUILD)/math_decompose_diff_impl.o \
                             $(BUILD)/errno.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^ -lm

math_special_test_run: $(BUILD)/special_probe $(BUILD)/special_differential \
                       $(BUILD)/gamma_probe $(BUILD)/gamma_differential
	@test "$$($(BUILD)/special_probe)" = "special-ok" || { echo "unexpected special-function probe output" >&2; exit 1; }
	$(BUILD)/special_differential
	@test "$$($(BUILD)/gamma_probe)" = "gamma-ok" || { echo "unexpected gamma probe output" >&2; exit 1; }
	$(BUILD)/gamma_differential

math_special_inspect: $(BUILD)/special_probe $(BUILD)/gamma_probe
	./tests/verify-no-host-libc.sh $(BUILD)/special_probe
	./tests/verify-no-host-libc.sh $(BUILD)/gamma_probe
