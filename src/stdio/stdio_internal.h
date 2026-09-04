#ifndef MINI_LIBC_STDIO_INTERNAL_H
#define MINI_LIBC_STDIO_INTERNAL_H

#include <stdio.h>

#define MINI_FILE_READABLE 1U
#define MINI_FILE_WRITABLE 2U
#define MINI_FILE_OWNED 4U

#define MINI_FILE_EOF 4U
#define MINI_FILE_ERROR 8U

struct __mini_FILE {
    int fd;
    unsigned int mode;
    unsigned int state;
};

#endif
