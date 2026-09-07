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

$(LIBC): $(BUILD)/fenv.o $(BUILD)/fenv_asm.o
all: $(BUILD)/fenv_probe $(BUILD)/fenv_interop
inspect: fenv_inspect
test: fenv_test_run

.PHONY: fenv_inspect fenv_test_run

$(BUILD)/fenv.o: src/fenv/fenv.c include/fenv.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/fenv_asm.o: src/fenv/fenv_asm.S | $(BUILD)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD)/fenv_probe.o: tests/fenv_probe.c include/fenv.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/fenv_probe: $(BUILD)/fenv_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/fenv_probe.o $(CRT0) $(LIBC)

$(BUILD)/fenv_test_impl.o: src/fenv/fenv.c include/fenv.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(FENV_RENAMES) -c $< -o $@

$(BUILD)/fenv_interop.o: tests/fenv_interop.c | $(BUILD)
	$(CC) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/fenv_interop: $(BUILD)/fenv_interop.o $(BUILD)/fenv_test_impl.o $(BUILD)/fenv_asm.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^ -lm

fenv_test_run: $(BUILD)/fenv_probe $(BUILD)/fenv_interop
	@test "$$($(BUILD)/fenv_probe)" = "fenv-ok" || { echo "unexpected fenv probe output" >&2; exit 1; }
	@test "$$($(BUILD)/fenv_interop)" = "fenv-interop-ok" || { echo "unexpected fenv interop output" >&2; exit 1; }

fenv_inspect: $(BUILD)/fenv_probe
	./tests/verify-no-host-libc.sh $(BUILD)/fenv_probe
