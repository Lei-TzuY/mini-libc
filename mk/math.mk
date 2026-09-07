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
                -Dsin=mini_test_sin -Dsinf=mini_test_sinf \
                -Dcos=mini_test_cos -Dcosf=mini_test_cosf \
                -Dtan=mini_test_tan -Dtanf=mini_test_tanf \
                -Datan=mini_test_atan -Datanf=mini_test_atanf \
                -Datan2=mini_test_atan2 -Datan2f=mini_test_atan2f \
                -Dasin=mini_test_asin -Dasinf=mini_test_asinf \
                -Dacos=mini_test_acos -Dacosf=mini_test_acosf \
                -Dsqrt=mini_test_sqrt -Dsqrtf=mini_test_sqrtf

$(LIBC): $(BUILD)/math.o $(BUILD)/math_decompose.o $(BUILD)/math_explog.o $(BUILD)/math_pow.o $(BUILD)/math_trig.o $(BUILD)/math_inverse_trig.o $(BUILD)/math_sqrt.o
all: $(BUILD)/math_probe $(BUILD)/math_differential $(BUILD)/explog_probe $(BUILD)/explog_differential $(BUILD)/pow_probe $(BUILD)/pow_differential $(BUILD)/trig_probe $(BUILD)/trig_differential $(BUILD)/inverse_trig_probe $(BUILD)/inverse_trig_differential
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

$(BUILD)/math_trig.o: src/math/trig.c include/math.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/math_inverse_trig.o: src/math/inverse_trig.c include/math.h include/errno.h | $(BUILD)
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

$(BUILD)/trig_probe.o: tests/trig_probe.c include/math.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/trig_probe: $(BUILD)/trig_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/trig_probe.o $(CRT0) $(LIBC)

$(BUILD)/inverse_trig_probe.o: tests/inverse_trig_probe.c include/math.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/inverse_trig_probe: $(BUILD)/inverse_trig_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/inverse_trig_probe.o $(CRT0) $(LIBC)

$(BUILD)/math_diff_impl.o: src/math/math.c include/math.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(MATH_RENAMES) -c $< -o $@

$(BUILD)/math_decompose_diff_impl.o: src/math/decompose.c include/math.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(MATH_RENAMES) -c $< -o $@

$(BUILD)/math_explog_diff_impl.o: src/math/explog.c include/math.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(MATH_RENAMES) -c $< -o $@

$(BUILD)/math_pow_diff_impl.o: src/math/pow.c include/math.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(MATH_RENAMES) -c $< -o $@

$(BUILD)/math_trig_diff_impl.o: src/math/trig.c include/math.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(MATH_RENAMES) -c $< -o $@

$(BUILD)/math_inverse_trig_diff_impl.o: src/math/inverse_trig.c include/math.h include/errno.h | $(BUILD)
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

$(BUILD)/trig_differential.o: tests/trig_differential.c | $(BUILD)
	$(CC) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/trig_differential: $(BUILD)/trig_differential.o $(BUILD)/math_trig_diff_impl.o $(BUILD)/errno.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^ -lm

$(BUILD)/inverse_trig_differential.o: tests/inverse_trig_differential.c | $(BUILD)
	$(CC) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/inverse_trig_differential: $(BUILD)/inverse_trig_differential.o $(BUILD)/math_inverse_trig_diff_impl.o $(BUILD)/math_diff_impl.o $(BUILD)/math_sqrt.o $(BUILD)/errno.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^ -lm

math_test_run: $(BUILD)/math_probe $(BUILD)/math_differential $(BUILD)/explog_probe $(BUILD)/explog_differential $(BUILD)/pow_probe $(BUILD)/pow_differential $(BUILD)/trig_probe $(BUILD)/trig_differential $(BUILD)/inverse_trig_probe $(BUILD)/inverse_trig_differential
	@test "$$($(BUILD)/math_probe)" = "math-ok" || { echo "unexpected math probe output" >&2; exit 1; }
	@test "$$($(BUILD)/explog_probe)" = "explog-ok" || { echo "unexpected exp/log probe output" >&2; exit 1; }
	@test "$$($(BUILD)/pow_probe)" = "pow-ok" || { echo "unexpected pow probe output" >&2; exit 1; }
	@test "$$($(BUILD)/trig_probe)" = "trig-ok" || { echo "unexpected trig probe output" >&2; exit 1; }
	@test "$$($(BUILD)/inverse_trig_probe)" = "inverse-trig-ok" || { echo "unexpected inverse trig probe output" >&2; exit 1; }
	$(BUILD)/math_differential
	$(BUILD)/explog_differential
	$(BUILD)/pow_differential
	$(BUILD)/trig_differential
	$(BUILD)/inverse_trig_differential

math_inspect: $(BUILD)/math_probe $(BUILD)/explog_probe $(BUILD)/pow_probe $(BUILD)/trig_probe $(BUILD)/inverse_trig_probe
	./tests/verify-no-host-libc.sh $(BUILD)/math_probe
	./tests/verify-no-host-libc.sh $(BUILD)/explog_probe
	./tests/verify-no-host-libc.sh $(BUILD)/pow_probe
	./tests/verify-no-host-libc.sh $(BUILD)/trig_probe
	./tests/verify-no-host-libc.sh $(BUILD)/inverse_trig_probe
