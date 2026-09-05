$(LIBC): $(BUILD)/setjmp.o
all: $(BUILD)/setjmp_probe
inspect: setjmp_inspect
test: setjmp_test_run

.PHONY: setjmp_inspect setjmp_test_run

$(BUILD)/setjmp.o: src/control/setjmp.S include/setjmp.h | $(BUILD)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD)/setjmp_probe.o: tests/setjmp_probe.c include/setjmp.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/setjmp_register_probe.o: tests/setjmp_register_probe.S | $(BUILD)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD)/setjmp_probe: $(BUILD)/setjmp_probe.o $(BUILD)/setjmp_register_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/setjmp_probe.o \
		$(BUILD)/setjmp_register_probe.o $(CRT0) $(LIBC)

setjmp_test_run: $(BUILD)/setjmp_probe
	@test "$$($(BUILD)/setjmp_probe)" = "setjmp-ok"

setjmp_inspect: $(BUILD)/setjmp_probe
	./tests/verify-no-host-libc.sh $(BUILD)/setjmp_probe
