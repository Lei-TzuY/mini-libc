FILE_SYNC_RENAMES := -Dsetvbuf=__mini_setvbuf_unlocked \
                     -Dsetbuf=__mini_setbuf_unlocked \
                     -Dfreopen=__mini_freopen_unlocked \
                     -Dfclose=__mini_fclose_unlocked
POSITION_SYNC_RENAMES := -Dfseek=__mini_fseek_unlocked \
                         -Dftell=__mini_ftell_unlocked \
                         -Drewind=__mini_rewind_unlocked
FORMAT_SYNC_RENAMES := -D__mini_format_dispatch=__mini_format_dispatch_unlocked
SCAN_SYNC_RENAMES := -D__mini_scan_dispatch=__mini_scan_dispatch_unlocked

$(BUILD)/file_stream.o $(BUILD)/file_stream_test_impl.o: CFLAGS += $(FILE_SYNC_RENAMES)
$(BUILD)/position.o $(BUILD)/position_test_impl.o: CFLAGS += $(POSITION_SYNC_RENAMES)
$(BUILD)/format.o: CFLAGS += $(FORMAT_SYNC_RENAMES)
$(BUILD)/scan.o $(BUILD)/scan_test_impl.o: CFLAGS += $(SCAN_SYNC_RENAMES)

$(LIBC): $(BUILD)/file_sync.o $(BUILD)/position_sync.o $(BUILD)/format_sync.o $(BUILD)/scan_sync.o

$(BUILD)/file_sync.o: src/stdio/file_sync.c src/stdio/stdio_internal.h include/stdio.h include/stddef.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/position_sync.o: src/stdio/position_sync.c src/stdio/stdio_internal.h include/stdio.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/format_sync.o: src/stdio/format_sync.c src/stdio/stdio_internal.h include/stdio.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/scan_sync.o: src/stdio/scan_sync.c src/stdio/stdio_internal.h include/stdio.h | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/stdio_lock_fake.o: tests/stdio_lock_fake.c | $(BUILD)
	$(CC) $(HOST_CFLAGS) -c $< -o $@

$(BUILD)/stdio_write_test: $(BUILD)/file_sync.o $(BUILD)/stdio_lock_fake.o
$(BUILD)/stdio_block_test: $(BUILD)/position_sync.o $(BUILD)/stdio_lock_fake.o
$(BUILD)/stdio_scan_test: $(BUILD)/scan_sync.o $(BUILD)/stdio_lock_fake.o
