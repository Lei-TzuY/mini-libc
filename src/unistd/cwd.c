#include <errno.h>
#include <mini/syscall.h>
#include <stddef.h>
#include <unistd.h>

char *getcwd(char *buf, size_t size)
{
    long result;

    if (buf == (char *)0 || size == 0U) {
        errno = EINVAL;
        return (char *)0;
    }

    result = mini_sys_getcwd(buf, (unsigned long)size);
    if (result < 0L) {
        errno = (int)-result;
        return (char *)0;
    }
    return buf;
}

int chdir(const char *path)
{
    long result;

    if (path == (const char *)0) {
        errno = EINVAL;
        return -1;
    }

    result = mini_sys_chdir(path);
    if (result < 0L) {
        errno = (int)-result;
        return -1;
    }
    return 0;
}

int fchdir(int fd)
{
    long result = mini_sys_fchdir(fd);

    if (result < 0L) {
        errno = (int)-result;
        return -1;
    }
    return 0;
}
