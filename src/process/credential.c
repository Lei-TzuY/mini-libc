#include <errno.h>
#include <mini/syscall.h>
#include <sys/types.h>
#include <unistd.h>

uid_t getuid(void)
{
    return (uid_t)mini_sys_getuid();
}

uid_t geteuid(void)
{
    return (uid_t)mini_sys_geteuid();
}

gid_t getgid(void)
{
    return (gid_t)mini_sys_getgid();
}

gid_t getegid(void)
{
    return (gid_t)mini_sys_getegid();
}

int setuid(uid_t uid)
{
    int saved_errno = errno;
    long result = mini_sys_setuid((unsigned int)uid);

    if (result < 0L) {
        errno = (int)-result;
        return -1;
    }
    errno = saved_errno;
    return 0;
}

int setgid(gid_t gid)
{
    int saved_errno = errno;
    long result = mini_sys_setgid((unsigned int)gid);

    if (result < 0L) {
        errno = (int)-result;
        return -1;
    }
    errno = saved_errno;
    return 0;
}
