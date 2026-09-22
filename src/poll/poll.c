#include <errno.h>
#include <mini/syscall.h>
#include <poll.h>

int poll(struct pollfd fds[], nfds_t nfds, int timeout)
{
    long result = mini_sys_poll(fds, (unsigned long)nfds, timeout);

    if (result < 0L) {
        errno = (int)-result;
        return -1;
    }
    return (int)result;
}
