#ifndef MINI_LIBC_ERRNO_H
#define MINI_LIBC_ERRNO_H

/* Linux x86-64 errno values used by implemented libc routines. */
#define ENOMEM 12
#define EINVAL 22
#define ERANGE 34

/*
 * ISO C requires errno to expand to a modifiable int lvalue. The accessor
 * keeps that source-level contract stable when process-global storage is
 * eventually replaced by thread-local storage.
 */
int *__mini_errno_location(void);
#define errno (*__mini_errno_location())

#endif
