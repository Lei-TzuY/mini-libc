#ifndef MINI_LIBC_SYS_WAIT_H
#define MINI_LIBC_SYS_WAIT_H

#include <sys/types.h>

#define WNOHANG 1

#define WIFEXITED(status) (((status) & 0x7f) == 0)
#define WEXITSTATUS(status) (((status) >> 8) & 0xff)

pid_t waitpid(pid_t pid, int *status, int options);

#endif
