#ifndef MINI_LIBC_UNISTD_H
#define MINI_LIBC_UNISTD_H

#include <stddef.h>
#include <sys/types.h>

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
int close(int fd);
char *getcwd(char *buf, size_t size);
int chdir(const char *path);
int fchdir(int fd);
int pipe(int pipefd[2]);
int pipe2(int pipefd[2], int flags);
pid_t fork(void);
int dup(int oldfd);
int dup2(int oldfd, int newfd);
off_t lseek(int fd, off_t offset, int whence);
int ftruncate(int fd, off_t length);
int unlink(const char *path);
int unlinkat(int dirfd, const char *path, int flags);

#endif
