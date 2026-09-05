#ifndef MINI_LIBC_STDIO_INTERNAL_H
#define MINI_LIBC_STDIO_INTERNAL_H

#include <stdio.h>

#ifndef MINI_STDIO_SYNC_PUBLIC_WRAPPER
#define setvbuf __mini_setvbuf_unlocked
#define setbuf __mini_setbuf_unlocked
#define freopen __mini_freopen_unlocked
#define fclose __mini_fclose_unlocked
#define fseek __mini_fseek_unlocked
#define ftell __mini_ftell_unlocked
#define rewind __mini_rewind_unlocked
#define __mini_format_dispatch __mini_format_dispatch_unlocked
#define __mini_scan_dispatch __mini_scan_dispatch_unlocked
#endif

#define MINI_FILE_READABLE 1U
#define MINI_FILE_WRITABLE 2U
#define MINI_FILE_OWNED 4U
#define MINI_FILE_UNBUFFERED 8U
#define MINI_FILE_APPEND 16U
#define MINI_FILE_LINE_BUFFERED 32U
#define MINI_FILE_BUFFER_OWNED 64U

#define MINI_FILE_EOF 4U
#define MINI_FILE_ERROR 8U
#define MINI_FILE_READ_NEEDS_POSITION 16U
#define MINI_FILE_WRITE_NEEDS_SYNC 32U

#define MINI_FILE_BUFFER_SIZE 256U

struct __mini_FILE {
    int fd;
    unsigned int mode;
    unsigned int state;
    FILE *next;
    size_t write_length;
    unsigned char *write_buffer;
    size_t read_offset;
    size_t read_length;
    unsigned int pushback_valid;
    unsigned char pushback_byte;
    unsigned char *read_buffer;
    size_t buffer_size;
    unsigned char inline_write_buffer[MINI_FILE_BUFFER_SIZE];
    unsigned char inline_read_buffer[MINI_FILE_BUFFER_SIZE];
};

struct mini_format_args;
struct mini_scan_args;

int __mini_setvbuf_unlocked(FILE *restrict stream, char *restrict buf,
                            int mode, size_t size);
void __mini_setbuf_unlocked(FILE *restrict stream, char *restrict buf);
FILE *__mini_freopen_unlocked(const char *restrict filename,
                              const char *restrict mode,
                              FILE *restrict stream);
int __mini_fclose_unlocked(FILE *stream);
int __mini_fseek_unlocked(FILE *stream, long offset, int whence);
long __mini_ftell_unlocked(FILE *stream);
void __mini_rewind_unlocked(FILE *stream);
int __mini_format_dispatch_unlocked(FILE *stream, const char *format,
                                    struct mini_format_args *args);
int __mini_scan_dispatch_unlocked(FILE *stream, const char *format,
                                  struct mini_scan_args *args);

void __mini_stdio_lock(void);
void __mini_stdio_unlock(void);
size_t __mini_stdio_read(FILE *stream, unsigned char *buffer, size_t length);
size_t __mini_stdio_write(FILE *stream, const unsigned char *buffer,
                          size_t length);
int __mini_stdio_flush_buffer(FILE *stream);
int __mini_stdio_flush_all(void);
void __mini_stdio_register(FILE *stream);
void __mini_stdio_unregister(FILE *stream);
void __mini_stdio_release_buffer(FILE *stream);

#endif
