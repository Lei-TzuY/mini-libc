MATH_RENAMES := -Dfabs=mini_test_fabs -Dfabsf=mini_test_fabsf \
                -Dcopysign=mini_test_copysign -Dcopysignf=mini_test_copysignf \
                -Dfmin=mini_test_fmin -Dfminf=mini_test_fminf \
                -Dfmax=mini_test_fmax -Dfmaxf=mini_test_fmaxf \
                -Dtrunc=mini_test_trunc -Dtruncf=mini_test_truncf \
                -Dfloor=mini_test_floor -Dfloorf=mini_test_floorf \
                -Dceil=mini_test_ceil -Dceilf=mini_test_ceilf \
                -Dround=mini_test_round -Droundf=mini_test_roundf \
                -Dfrexp=mini_test_frexp -Dfrexpf=mini_test_frexpf \
                -Dldexp=mini_test_ldexp -Dldexpf=mini_test_ldexpf \
                -Dscalbn=mini_test_scalbn -Dscalbnf=mini_test_scalbnf \
                -Dmodf=mini_test_modf -Dmodff=mini_test_modff \
                -Dexp=mini_test_exp -Dexpf=mini_test_expf \
                -Dlog=mini_test_log -Dlogf=mini_test_logf \
                -Dpow=mini_test_pow -Dpowf=mini_test_powf \
                -Dsqrt=mini_test_sqrt -Dsqrtf=mini_test_sqrtf

$(LIBC): $(BUILD)/math.o $(BUILD)/math_decompose.o $(BUILD)/math_explog.o $(BUILD)/math_pow.o $(BUILD)/math_sqrt.o
all: $(BUILD)/math_probe $(BUILD)/math_differential $(BUILD)/explog_probe $(BUILD)/explog_differential $(BUILD)/pow_probe $(BUILD)/pow_differential
inspect: math_inspect
test: math_test_run

.PHONY: math_inspect math_test_run

$(BUILD)/math.o: src/math/math.c include/math.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/math_decompose.o: src/math/decompose.c include/math.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/math_explog.o: src/math/explog.c include/math.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/math_pow.o: src/math/pow.c include/math.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/math_sqrt.o: src/math/sqrt.S | $(BUILD)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD)/math_probe.o: tests/math_probe.c include/math.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/math_probe: $(BUILD)/math_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/math_probe.o $(CRT0) $(LIBC)

$(BUILD)/explog_probe.o: tests/explog_probe.c include/math.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/explog_probe: $(BUILD)/explog_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/explog_probe.o $(CRT0) $(LIBC)

$(BUILD)/pow_probe.o: tests/pow_probe.c include/math.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/pow_probe: $(BUILD)/pow_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/pow_probe.o $(CRT0) $(LIBC)

$(BUILD)/math_diff_impl.o: src/math/math.c include/math.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(MATH_RENAMES) -c $< -o $@

$(BUILD)/math_decompose_diff_impl.o: src/math/decompose.c include/math.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(MATH_RENAMES) -c $< -o $@

$(BUILD)/math_explog_diff_impl.o: src/math/explog.c include/math.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(MATH_RENAMES) -c $< -o $@

$(BUILD)/math_pow_diff_impl.o: src/math/pow.c include/math.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(MATH_RENAMES) -c $< -o $@

$(BUILD)/math_differential.o: tests/math_differential.c | $(BUILD)
	$(CC) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/math_differential: $(BUILD)/math_differential.o $(BUILD)/math_diff_impl.o $(BUILD)/math_decompose_diff_impl.o $(BUILD)/math_sqrt.o $(BUILD)/errno.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^ -lm

$(BUILD)/explog_differential.o: tests/explog_differential.c | $(BUILD)
	$(CC) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/explog_differential: $(BUILD)/explog_differential.o $(BUILD)/math_explog_diff_impl.o $(BUILD)/math_decompose_diff_impl.o $(BUILD)/errno.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^ -lm

$(BUILD)/pow_differential.o: tests/pow_differential.c | $(BUILD)
	$(CC) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/pow_differential: $(BUILD)/pow_differential.o $(BUILD)/math_pow_diff_impl.o $(BUILD)/math_explog_diff_impl.o $(BUILD)/math_decompose_diff_impl.o $(BUILD)/errno.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^ -lm

math_test_run: $(BUILD)/math_probe $(BUILD)/math_differential $(BUILD)/explog_probe $(BUILD)/explog_differential $(BUILD)/pow_probe $(BUILD)/pow_differential
	@test "$$($(BUILD)/math_probe)" = "math-ok" || { echo "unexpected math probe output" >&2; exit 1; }
	@test "$$($(BUILD)/explog_probe)" = "explog-ok" || { echo "unexpected exp/log probe output" >&2; exit 1; }
	@test "$$($(BUILD)/pow_probe)" = "pow-ok" || { echo "unexpected pow probe output" >&2; exit 1; }
	$(BUILD)/math_differential
	$(BUILD)/explog_differential
	$(BUILD)/pow_differential

math_inspect: $(BUILD)/math_probe $(BUILD)/explog_probe $(BUILD)/pow_probe
	./tests/verify-no-host-libc.sh $(BUILD)/math_probe
	./tests/verify-no-host-libc.sh $(BUILD)/explog_probe
	./tests/verify-no-host-libc.sh $(BUILD)/pow_probe
