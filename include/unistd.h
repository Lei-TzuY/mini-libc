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
uid_t getuid(void);
uid_t geteuid(void);
gid_t getgid(void);
gid_t getegid(void);
int setuid(uid_t uid);
int setgid(gid_t gid);
pid_t getpid(void);
pid_t getppid(void);
pid_t getpgrp(void);
pid_t getpgid(pid_t pid);
int setpgid(pid_t pid, pid_t pgid);
pid_t getsid(pid_t pid);
pid_t setsid(void);
pid_t tcgetpgrp(int fd);
int tcsetpgrp(int fd, pid_t pgrp);
pid_t fork(void);
pid_t _Fork(void);
int execve(const char *path, char *const argv[], char *const envp[]);
int dup(int oldfd);
int dup2(int oldfd, int newfd);
off_t lseek(int fd, off_t offset, int whence);
int ftruncate(int fd, off_t length);
int unlink(const char *path);
int unlinkat(int dirfd, const char *path, int flags);

#endif
