#include <errno.h>
#include <fcntl.h>
#include <mini/syscall.h>
#include <stdarg.h>
#include <unistd.h>

static int translate_fd_result(long result)
{
    if (result < 0L) {
        errno = (int)-result;
        return -1;
    }
    return (int)result;
}

int dup(int oldfd)
{
    return translate_fd_result(mini_sys_dup(oldfd));
}

int dup2(int oldfd, int newfd)
{
    return translate_fd_result(mini_sys_dup2(oldfd, newfd));
}

int fcntl(int fd, int cmd, ...)
{
    unsigned long argument = 0UL;
    long result;

    if (cmd == F_DUPFD || cmd == F_SETFD) {
        va_list args;

        va_start(args, cmd);
        argument = (unsigned long)va_arg(args, int);
        va_end(args);
    } else if (cmd != F_GETFD && cmd != F_GETFL) {
        errno = EINVAL;
        return -1;
    }

    result = mini_sys_fcntl(fd, cmd, argument);
    return translate_fd_result(result);
}
