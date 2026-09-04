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
long mini_sys_lseek(int fd, long offset, int whence);
long mini_sys_brk(void *addr);
long mini_sys_mmap(void *addr, unsigned long length, int prot, int flags,
                   int fd, long offset);
long mini_sys_munmap(void *addr, unsigned long length);
_Noreturn void mini_sys_exit(int status);

#endif
