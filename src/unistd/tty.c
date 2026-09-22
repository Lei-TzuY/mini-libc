#include <errno.h>
#include <mini/syscall.h>
#include <mini/tty_ioctl.h>
#include <sys/types.h>
#include <termios.h>
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

int tcgetattr(int fd, struct termios *termios_p)
{
    int saved_errno = errno;
    long result = mini_sys_ioctl(fd, MINI_TCGETS,
                                 (unsigned long)termios_p);

    if (result < 0L) {
        errno = (int)-result;
        return -1;
    }
    errno = saved_errno;
    return 0;
}

int tcsetattr(int fd, int optional_actions,
              const struct termios *termios_p)
{
    unsigned long request;
    int saved_errno = errno;
    long result;

    if (optional_actions == TCSANOW) {
        request = MINI_TCSETS;
    } else if (optional_actions == TCSADRAIN) {
        request = MINI_TCSETSW;
    } else if (optional_actions == TCSAFLUSH) {
        request = MINI_TCSETSF;
    } else {
        errno = EINVAL;
        return -1;
    }

    result = mini_sys_ioctl(fd, request, (unsigned long)termios_p);
    if (result < 0L) {
        errno = (int)-result;
        return -1;
    }
    errno = saved_errno;
    return 0;
}
