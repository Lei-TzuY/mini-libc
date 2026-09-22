#include <errno.h>
#include <mini/syscall.h>
#include <sys/resource.h>

int getrlimit(int resource, struct rlimit *rlim)
{
    int saved_errno = errno;
    long result = mini_sys_getrlimit(resource, rlim);

    if (result < 0L) {
        errno = (int)-result;
        return -1;
    }
    errno = saved_errno;
    return 0;
}

int setrlimit(int resource, const struct rlimit *rlim)
{
    int saved_errno = errno;
    long result = mini_sys_setrlimit(resource, rlim);

    if (result < 0L) {
        errno = (int)-result;
        return -1;
    }
    errno = saved_errno;
    return 0;
}
