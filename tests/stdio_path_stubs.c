#include <errno.h>

long mini_sys_unlinkat(int dirfd, const char *path, int flags)
{
    (void)dirfd;
    (void)path;
    (void)flags;
    return -EINVAL;
}

long mini_sys_renameat(int olddirfd, const char *oldpath,
                       int newdirfd, const char *newpath)
{
    (void)olddirfd;
    (void)oldpath;
    (void)newdirfd;
    (void)newpath;
    return -EINVAL;
}
