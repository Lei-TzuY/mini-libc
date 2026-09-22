#ifndef MINI_LIBC_SYS_WAIT_H
#define MINI_LIBC_SYS_WAIT_H

#include <sys/types.h>

#define WNOHANG 1

#define WTERMSIG(status) ((status) & 0x7f)
#define WIFEXITED(status) (WTERMSIG(status) == 0)
#define WEXITSTATUS(status) (((status) >> 8) & 0xff)
#define WIFSIGNALED(status) \
    (WTERMSIG(status) != 0 && WTERMSIG(status) != 0x7f)

pid_t waitpid(pid_t pid, int *status, int options);

#endif
