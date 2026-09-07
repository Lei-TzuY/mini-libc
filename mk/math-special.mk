SPECIAL_RENAMES := $(MATH_RENAMES) \
                   -Derf=mini_test_erf -Derff=mini_test_erff \
                   -Derfc=mini_test_erfc -Derfcf=mini_test_erfcf

$(LIBC): $(BUILD)/math_special.o
all: $(BUILD)/special_probe $(BUILD)/special_differential
inspect: math_special_inspect
test: math_special_test_run

.PHONY: math_special_inspect math_special_test_run

$(BUILD)/math_special.o: src/math/special.c include/math.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/special_probe.o: tests/special_probe.c include/math.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/special_probe: $(BUILD)/special_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/special_probe.o $(CRT0) $(LIBC)

$(BUILD)/math_special_diff_impl.o: src/math/special.c include/math.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SPECIAL_RENAMES) -c $< -o $@

$(BUILD)/special_differential.o: tests/special_differential.c | $(BUILD)
	$(CC) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/special_differential: $(BUILD)/special_differential.o \
                               $(BUILD)/math_special_diff_impl.o \
                               $(BUILD)/math_explog_diff_impl.o \
                               $(BUILD)/math_decompose_diff_impl.o \
                               $(BUILD)/errno.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^ -lm

math_special_test_run: $(BUILD)/special_probe $(BUILD)/special_differential
	@test "$$($(BUILD)/special_probe)" = "special-ok" || { echo "unexpected special-function probe output" >&2; exit 1; }
	$(BUILD)/special_differential

math_special_inspect: $(BUILD)/special_probe
	./tests/verify-no-host-libc.sh $(BUILD)/special_probe
