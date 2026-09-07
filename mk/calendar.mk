$(LIBC): $(BUILD)/calendar.o

$(BUILD)/calendar.o: src/time/calendar.c include/time.h include/stddef.h include/errno.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/time_probe.o: include/threads.h

$(BUILD)/time_test: $(BUILD)/calendar.o
