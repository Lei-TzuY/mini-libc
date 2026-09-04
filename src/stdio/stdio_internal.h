#ifndef MINI_LIBC_STDIO_INTERNAL_H
#define MINI_LIBC_STDIO_INTERNAL_H

#include <stdio.h>

#define MINI_FILE_READABLE 1U
#define MINI_FILE_WRITABLE 2U
#define MINI_FILE_OWNED 4U
#define MINI_FILE_UNBUFFERED 8U

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
    unsigned char write_buffer[MINI_FILE_BUFFER_SIZE];
};

size_t __mini_stdio_write(FILE *stream, const unsigned char *buffer,
                          size_t length);
int __mini_stdio_flush_buffer(FILE *stream);
int __mini_stdio_flush_all(void);
void __mini_stdio_register(FILE *stream);
void __mini_stdio_unregister(FILE *stream);

#endif
