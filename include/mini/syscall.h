#ifndef MINI_LIBC_MINI_SYSCALL_H
#define MINI_LIBC_MINI_SYSCALL_H

/*
 * Raw Linux x86-64 syscall boundary.
 *
 * These functions deliberately expose kernel return values directly. Most
 * failures are negative errno values in [-4095, -1], but raw Linux brk is an
 * important exception: it returns the resulting program break and reports a
 * refused request by returning the unchanged break. None of these wrappers are
 * POSIX libc wrappers, and none set errno.
 */

long mini_sys_read(int fd, void *buf, unsigned long count);
long mini_sys_write(int fd, const void *buf, unsigned long count);
long mini_sys_close(int fd);
long mini_sys_dup(int oldfd);
long mini_sys_dup2(int oldfd, int newfd);
long mini_sys_fstat(int fd, void *statbuf);
long mini_sys_fcntl(int fd, int cmd, unsigned long arg);
long mini_sys_ioctl(int fd, unsigned long request, unsigned long arg);
long mini_sys_lseek(int fd, long offset, int whence);
long mini_sys_getcwd(char *buf, unsigned long size);
long mini_sys_chdir(const char *path);
long mini_sys_fchdir(int fd);
long mini_sys_fork(void);
long mini_sys_execve(const char *path, char *const argv[], char *const envp[]);
long mini_sys_wait4(int pid, int *status, int options, void *rusage);
long mini_sys_sched_yield(void);
long mini_sys_nanosleep(const void *request, void *remaining);
long mini_sys_rt_sigaction(int sig, const void *act, void *oldact,
                           unsigned long sigsetsize);
long mini_sys_getpid(void);
long mini_sys_getuid(void);
long mini_sys_getgid(void);
long mini_sys_setuid(unsigned int uid);
long mini_sys_setgid(unsigned int gid);
long mini_sys_geteuid(void);
long mini_sys_getegid(void);
long mini_sys_getppid(void);
long mini_sys_kill(int pid, int sig);
long mini_sys_setpgid(int pid, int pgid);
long mini_sys_getpgid(int pid);
long mini_sys_setsid(void);
long mini_sys_getsid(int pid);
long mini_sys_getrlimit(int resource, void *rlim);
long mini_sys_setrlimit(int resource, const void *rlim);
long mini_sys_arch_prctl(int code, unsigned long address);
long mini_sys_gettid(void);
long mini_sys_futex(volatile int *uaddr, int op, int value,
                    const void *timeout, volatile int *uaddr2, int value3);
long mini_sys_tgkill(int tgid, int tid, int sig);
long mini_sys_ftruncate(int fd, long length);
long mini_sys_getdents64(int fd, void *dirp, unsigned long count);
long mini_sys_openat(int dirfd, const char *path, int flags, unsigned int mode);
long mini_sys_newfstatat(int dirfd, const char *path, void *statbuf, int flags);
long mini_sys_pipe2(int pipefd[2], int flags);
long mini_sys_poll(void *fds, unsigned long nfds, int timeout);
long mini_sys_unlinkat(int dirfd, const char *path, int flags);
long mini_sys_renameat(int olddirfd, const char *oldpath,
                       int newdirfd, const char *newpath);
long mini_sys_clock_gettime(int clockid, void *tp);
long mini_sys_brk(void *addr);
long mini_sys_mmap(void *addr, unsigned long length, int prot, int flags,
                   int fd, long offset);
long mini_sys_munmap(void *addr, unsigned long length);
_Noreturn void mini_sys_exit(int status);
_Noreturn void mini_sys_exit_group(int status);

#endif
