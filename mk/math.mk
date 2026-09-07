MATH_RENAMES := -Dfabs=mini_test_fabs -Dfabsf=mini_test_fabsf \
                -Dcopysign=mini_test_copysign -Dcopysignf=mini_test_copysignf \
                -Dfmin=mini_test_fmin -Dfminf=mini_test_fminf \
                -Dfmax=mini_test_fmax -Dfmaxf=mini_test_fmaxf \
                -Dtrunc=mini_test_trunc -Dtruncf=mini_test_truncf \
                -Dfloor=mini_test_floor -Dfloorf=mini_test_floorf \
                -Dceil=mini_test_ceil -Dceilf=mini_test_ceilf \
                -Dround=mini_test_round -Droundf=mini_test_roundf \
                -Dsqrt=mini_test_sqrt -Dsqrtf=mini_test_sqrtf

$(LIBC): $(BUILD)/math.o $(BUILD)/math_sqrt.o
all: $(BUILD)/math_probe $(BUILD)/math_differential
inspect: math_inspect
test: math_test_run

.PHONY: math_inspect math_test_run

$(BUILD)/math.o: src/math/math.c include/math.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/math_sqrt.o: src/math/sqrt.S | $(BUILD)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD)/math_probe.o: tests/math_probe.c include/math.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/math_probe: $(BUILD)/math_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/math_probe.o $(CRT0) $(LIBC)

$(BUILD)/math_diff_impl.o: src/math/math.c include/math.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(MATH_RENAMES) -c $< -o $@

$(BUILD)/math_differential.o: tests/math_differential.c | $(BUILD)
	$(CC) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/math_differential: $(BUILD)/math_differential.o $(BUILD)/math_diff_impl.o $(BUILD)/math_sqrt.o $(BUILD)/errno.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^ -lm

math_test_run: $(BUILD)/math_probe $(BUILD)/math_differential
	@test "$$($(BUILD)/math_probe)" = "math-ok" || { echo "unexpected math probe output" >&2; exit 1; }
	$(BUILD)/math_differential

math_inspect: $(BUILD)/math_probe
	./tests/verify-no-host-libc.sh $(BUILD)/math_probe
