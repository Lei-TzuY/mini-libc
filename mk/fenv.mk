FENV_RENAMES := -Dfeclearexcept=mini_test_feclearexcept \
                -Dfegetexceptflag=mini_test_fegetexceptflag \
                -Dferaiseexcept=mini_test_feraiseexcept \
                -Dfesetexceptflag=mini_test_fesetexceptflag \
                -Dfetestexcept=mini_test_fetestexcept \
                -Dfegetround=mini_test_fegetround \
                -Dfesetround=mini_test_fesetround \
                -Dfegetenv=mini_test_fegetenv \
                -Dfeholdexcept=mini_test_feholdexcept \
                -Dfesetenv=mini_test_fesetenv \
                -Dfeupdateenv=mini_test_feupdateenv

FENV_MATH_RENAMES := -Drint=mini_test_rint -Drintf=mini_test_rintf \
                     -Dnearbyint=mini_test_nearbyint \
                     -Dnearbyintf=mini_test_nearbyintf \
                     -Dlrint=mini_test_lrint -Dlrintf=mini_test_lrintf \
                     -Dllrint=mini_test_llrint -Dllrintf=mini_test_llrintf

$(LIBC): $(BUILD)/fenv.o $(BUILD)/fenv_asm.o $(BUILD)/math_fenv_rounding.o
all: $(BUILD)/fenv_probe $(BUILD)/fenv_interop $(BUILD)/fenv_rounding_interop \
     $(BUILD)/fenv_explog_probe $(BUILD)/fenv_explog_interop \
     $(BUILD)/fenv_pow_probe $(BUILD)/fenv_pow_interop
inspect: fenv_inspect
test: fenv_test_run

.PHONY: fenv_inspect fenv_test_run

$(BUILD)/fenv.o: src/fenv/fenv.c include/fenv.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/fenv_asm.o: src/fenv/fenv_asm.S | $(BUILD)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD)/math_fenv_rounding.o: src/math/fenv_rounding.c include/math.h include/fenv.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/fenv_probe.o: tests/fenv_probe.c include/fenv.h include/math.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/fenv_probe: $(BUILD)/fenv_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/fenv_probe.o $(CRT0) $(LIBC)

$(BUILD)/fenv_explog_probe.o: tests/fenv_explog_probe.c include/fenv.h include/math.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/fenv_explog_probe: $(BUILD)/fenv_explog_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/fenv_explog_probe.o $(CRT0) $(LIBC)

$(BUILD)/fenv_pow_probe.o: tests/fenv_pow_probe.c include/fenv.h include/math.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/fenv_pow_probe: $(BUILD)/fenv_pow_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/fenv_pow_probe.o $(CRT0) $(LIBC)

$(BUILD)/fenv_test_impl.o: src/fenv/fenv.c include/fenv.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(FENV_RENAMES) -c $< -o $@

$(BUILD)/fenv_rounding_test_impl.o: src/math/fenv_rounding.c include/math.h include/fenv.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(FENV_RENAMES) $(FENV_MATH_RENAMES) -c $< -o $@

$(BUILD)/fenv_explog_test_impl.o: src/math/explog.c include/math.h include/fenv.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(MATH_RENAMES) $(FENV_RENAMES) -c $< -o $@

$(BUILD)/fenv_pow_test_impl.o: src/math/pow.c include/math.h include/fenv.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(MATH_RENAMES) $(FENV_RENAMES) -c $< -o $@

$(BUILD)/fenv_interop.o: tests/fenv_interop.c | $(BUILD)
	$(CC) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/fenv_interop: $(BUILD)/fenv_interop.o $(BUILD)/fenv_test_impl.o $(BUILD)/fenv_asm.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^ -lm

$(BUILD)/fenv_rounding_interop.o: tests/fenv_rounding_interop.c | $(BUILD)
	$(CC) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/fenv_rounding_interop: $(BUILD)/fenv_rounding_interop.o $(BUILD)/fenv_rounding_test_impl.o $(BUILD)/fenv_test_impl.o $(BUILD)/fenv_asm.o $(BUILD)/errno.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^ -lm

$(BUILD)/fenv_explog_interop.o: tests/fenv_explog_interop.c | $(BUILD)
	$(CC) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/fenv_explog_interop: $(BUILD)/fenv_explog_interop.o $(BUILD)/fenv_explog_test_impl.o $(BUILD)/math_decompose_diff_impl.o $(BUILD)/fenv_test_impl.o $(BUILD)/fenv_asm.o $(BUILD)/errno.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^ -lm

$(BUILD)/fenv_pow_interop.o: tests/fenv_pow_interop.c | $(BUILD)
	$(CC) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/fenv_pow_interop: $(BUILD)/fenv_pow_interop.o $(BUILD)/fenv_pow_test_impl.o $(BUILD)/fenv_explog_test_impl.o $(BUILD)/math_decompose_diff_impl.o $(BUILD)/fenv_test_impl.o $(BUILD)/fenv_asm.o $(BUILD)/errno.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^ -lm

fenv_test_run: $(BUILD)/fenv_probe $(BUILD)/fenv_interop $(BUILD)/fenv_rounding_interop $(BUILD)/fenv_explog_probe $(BUILD)/fenv_explog_interop $(BUILD)/fenv_pow_probe $(BUILD)/fenv_pow_interop
	@test "$$($(BUILD)/fenv_probe)" = "fenv-ok" || { echo "unexpected fenv probe output" >&2; exit 1; }
	@test "$$($(BUILD)/fenv_interop)" = "fenv-interop-ok" || { echo "unexpected fenv interop output" >&2; exit 1; }
	@test "$$($(BUILD)/fenv_rounding_interop)" = "fenv-rounding-interop-ok" || { echo "unexpected fenv rounding interop output" >&2; exit 1; }
	@output=$$($(BUILD)/fenv_explog_probe); status=$$?; \
		test $$status -eq 0 -a "$$output" = "fenv-explog-ok" || { \
			echo "unexpected fenv explog probe: status=$$status output='$$output'" >&2; exit 1; }
	@test "$$($(BUILD)/fenv_explog_interop)" = "fenv-explog-interop-ok" || { echo "unexpected fenv explog interop output" >&2; exit 1; }
	@output=$$($(BUILD)/fenv_pow_probe); status=$$?; \
		test $$status -eq 0 -a "$$output" = "fenv-pow-ok" || { \
			echo "unexpected fenv pow probe: status=$$status output='$$output'" >&2; exit 1; }
	@test "$$($(BUILD)/fenv_pow_interop)" = "fenv-pow-interop-ok" || { echo "unexpected fenv pow interop output" >&2; exit 1; }

fenv_inspect: $(BUILD)/fenv_probe $(BUILD)/fenv_explog_probe $(BUILD)/fenv_pow_probe
	./tests/verify-no-host-libc.sh $(BUILD)/fenv_probe
	./tests/verify-no-host-libc.sh $(BUILD)/fenv_explog_probe
	./tests/verify-no-host-libc.sh $(BUILD)/fenv_pow_probe
