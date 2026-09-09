LOCALE_RENAMES := -Dsetlocale=mini_test_setlocale -Dlocaleconv=mini_test_localeconv
WCHAR_RENAMES := -Dmbsinit=mini_test_mbsinit -Dmbrtowc=mini_test_mbrtowc \
                 -Dwcrtomb=mini_test_wcrtomb -Dmbsrtowcs=mini_test_mbsrtowcs \
                 -Dwcsrtombs=mini_test_wcsrtombs -Dwcslen=mini_test_wcslen \
                 -Dwcscmp=mini_test_wcscmp -Dwcscpy=mini_test_wcscpy
MULTIBYTE_RENAMES := -Dmblen=mini_test_mblen -Dmbtowc=mini_test_mbtowc \
                     -Dwctomb=mini_test_wctomb -Dmbstowcs=mini_test_mbstowcs \
                     -Dwcstombs=mini_test_wcstombs

$(LIBC): $(BUILD)/locale.o $(BUILD)/wchar.o $(BUILD)/wide_stdio.o \
         $(BUILD)/wide_format.o $(BUILD)/multibyte.o
all: $(BUILD)/locale_probe $(BUILD)/locale_differential \
     $(BUILD)/wchar_probe $(BUILD)/wchar_differential \
     $(BUILD)/wide_stdio_probe
inspect: locale_inspect
test: locale_test_run

.PHONY: locale_inspect locale_test_run

$(BUILD)/locale.o: src/locale/locale.c include/locale.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/wchar.o: src/wchar/wchar.c include/wchar.h include/stddef.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/wide_stdio.o: src/wchar/wide_stdio.c include/wchar.h include/stdio.h \
                       src/stdio/stdio_internal.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/wide_format.o: src/wchar/wide_format.c include/wchar.h include/stdio.h \
                        include/stdarg.h include/stdlib.h include/errno.h \
                        src/stdio/stdio_internal.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/multibyte.o: src/stdlib/multibyte.c include/stdlib.h include/wchar.h include/stddef.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/locale_probe.o: tests/locale_probe.c include/locale.h include/stdlib.h include/stddef.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/locale_probe: $(BUILD)/locale_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/locale_probe.o $(CRT0) $(LIBC)

$(BUILD)/wchar_probe.o: tests/wchar_probe.c include/wchar.h include/stddef.h include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/wchar_probe: $(BUILD)/wchar_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/wchar_probe.o $(CRT0) $(LIBC)

$(BUILD)/wide_stdio_probe.o: tests/wide_stdio_probe.c include/wchar.h include/stdio.h \
                             include/errno.h include/mini/syscall.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/wide_stdio_probe: $(BUILD)/wide_stdio_probe.o $(CRT0) $(LIBC)
	$(LD) -static -e _start --build-id=none -o $@ $(BUILD)/wide_stdio_probe.o $(CRT0) $(LIBC)

$(BUILD)/locale_test_impl.o: src/locale/locale.c include/locale.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LOCALE_RENAMES) -c $< -o $@

$(BUILD)/wchar_test_impl.o: src/wchar/wchar.c include/wchar.h include/stddef.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WCHAR_RENAMES) -c $< -o $@

$(BUILD)/multibyte_test_impl.o: src/stdlib/multibyte.c include/stdlib.h include/wchar.h include/stddef.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(MULTIBYTE_RENAMES) $(WCHAR_RENAMES) -c $< -o $@

$(BUILD)/locale_differential.o: tests/locale_differential.c | $(BUILD)
	$(CC) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/locale_differential: $(BUILD)/locale_differential.o $(BUILD)/locale_test_impl.o $(BUILD)/multibyte_test_impl.o $(BUILD)/wchar_test_impl.o $(BUILD)/errno.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^

$(BUILD)/wchar_differential.o: tests/wchar_differential.c | $(BUILD)
	$(CC) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/wchar_differential: $(BUILD)/wchar_differential.o $(BUILD)/wchar_test_impl.o $(BUILD)/errno.o
	$(CC) $(HOST_LDFLAGS) -o $@ $^

locale_test_run: $(BUILD)/locale_probe $(BUILD)/locale_differential \
                 $(BUILD)/wchar_probe $(BUILD)/wchar_differential \
                 $(BUILD)/wide_stdio_probe
	@test "$$($(BUILD)/locale_probe)" = "locale-ok"
	@test "$$($(BUILD)/locale_differential)" = "locale-differential-ok"
	@test "$$($(BUILD)/wchar_probe)" = "wchar-ok"
	@test "$$($(BUILD)/wchar_differential)" = "wchar-differential-ok"
	@test "$$(printf 'ROW\n' | $(BUILD)/wide_stdio_probe)" = "!OKwide-stdio-ok"

locale_inspect: $(BUILD)/locale_probe $(BUILD)/wchar_probe $(BUILD)/wide_stdio_probe
	./tests/verify-no-host-libc.sh $(BUILD)/locale_probe
	./tests/verify-no-host-libc.sh $(BUILD)/wchar_probe
	./tests/verify-no-host-libc.sh $(BUILD)/wide_stdio_probe
