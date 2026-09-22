#include <errno.h>
#include <mini/syscall.h>
#include <mini/tty_ioctl.h>
#include <sys/types.h>
#include <unistd.h>

pid_t tcgetpgrp(int fd)
{
    int pgid = 0;
    int saved_errno = errno;
    long result = mini_sys_ioctl(fd, MINI_TIOCGPGRP,
                                 (unsigned long)&pgid);

    if (result < 0L) {
        errno = (int)-result;
        return (pid_t)-1;
    }
    errno = saved_errno;
    return (pid_t)pgid;
}

int tcsetpgrp(int fd, pid_t pgrp)
{
    int pgid = (int)pgrp;
    int saved_errno = errno;
    long result = mini_sys_ioctl(fd, MINI_TIOCSPGRP,
                                 (unsigned long)&pgid);

    if (result < 0L) {
        errno = (int)-result;
        return -1;
    }
    errno = saved_errno;
    return 0;
}
