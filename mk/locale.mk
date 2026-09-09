LOCALE_RENAMES := -Dsetlocale=mini_test_setlocale -Dlocaleconv=mini_test_localeconv
MULTIBYTE_RENAMES := -Dmblen=mini_test_mblen -Dmbtowc=mini_test_mbtowc \
                     -Dwctomb=mini_test_wctomb -Dmbstowcs=mini_test_mbstowcs \
                     -Dwcstombs=mini_test_wcstombs

$(LIBC): $(BUILD)/locale.o $(BUILD)/multibyte.o
all: $(BUILD)/locale_probe $(BUILD)/locale_differential
inspect: locale_inspect
test: locale_test_run

.PHONY: locale_inspect locale_test_run

$(BUILD)/locale.o: src/locale/locale.c include/locale.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/multibyte.o: src/stdlib/multibyte.c include/stdlib.h include/stddef.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/locale_probe.o: tests/locale_probe.c include/locale.h include/stdlib.h include/stddef.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/locale_probe: $(BUILD)/locale_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/locale_probe.o $(CRT0) $(LIBC)

$(BUILD)/locale_test_impl.o: src/locale/locale.c include/locale.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LOCALE_RENAMES) -c $< -o $@

$(BUILD)/multibyte_test_impl.o: src/stdlib/multibyte.c include/stdlib.h include/stddef.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(MULTIBYTE_RENAMES) -c $< -o $@

$(BUILD)/locale_differential.o: tests/locale_differential.c | $(BUILD)
	$(CC) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/locale_differential: $(BUILD)/locale_differential.o $(BUILD)/locale_test_impl.o $(BUILD)/multibyte_test_impl.o $(BUILD)/errno.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^

locale_test_run: $(BUILD)/locale_probe $(BUILD)/locale_differential
	@test "$$($(BUILD)/locale_probe)" = "locale-ok"
	@test "$$($(BUILD)/locale_differential)" = "locale-differential-ok"

locale_inspect: $(BUILD)/locale_probe
	./tests/verify-no-host-libc.sh $(BUILD)/locale_probe
