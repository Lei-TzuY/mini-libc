#ifndef MINI_LIBC_FCNTL_H
#define MINI_LIBC_FCNTL_H

#include <sys/types.h>

#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2
#define O_ACCMODE 3
#define O_CREAT 64
#define O_EXCL 128
#define O_TRUNC 512
#define O_APPEND 1024
#define O_DIRECTORY (1 << 16)
#define O_CLOEXEC (1 << 19)

#define AT_FDCWD (-100)
#define AT_REMOVEDIR 512

int open(const char *path, int oflag, ...);
int openat(int fd, const char *path, int oflag, ...);

#endif
