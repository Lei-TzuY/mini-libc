#ifndef MINI_LIBC_ERRNO_H
#define MINI_LIBC_ERRNO_H

/* Linux x86-64 errno values used by implemented libc routines. */
#define EPERM 1
#define ENOENT 2
#define ESRCH 3
#define EIO 5
#define EBADF 9
#define ECHILD 10
#define EAGAIN 11
#define ENOMEM 12
#define EEXIST 17
#define ENOTDIR 20
#define EISDIR 21
#define EINVAL 22
#define ENOTTY 25
#define EFBIG 27
#define EPIPE 32
#define EDOM 33
#define ERANGE 34
#define EILSEQ 84

/*
 * ISO C requires errno to expand to a modifiable int lvalue. The accessor
 * keeps that source-level contract stable when process-global storage is
 * eventually replaced by thread-local storage.
 */
int *__mini_errno_location(void);
#define errno (*__mini_errno_location())

#endif
