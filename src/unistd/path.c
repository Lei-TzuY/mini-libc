#include <errno.h>
#include <fcntl.h>
#include <mini/syscall.h>
#include <stdio.h>
#include <unistd.h>

static long unlinkat_raw(int dirfd, const char *path, int flags)
{
    return mini_sys_unlinkat(dirfd, path, flags);
}

static int translate_zero_result(long result)
{
    if (result < 0L) {
        errno = (int)-result;
        return -1;
    }
    return 0;
}

int unlinkat(int dirfd, const char *path, int flags)
{
    return translate_zero_result(unlinkat_raw(dirfd, path, flags));
}

int unlink(const char *path)
{
    return unlinkat(AT_FDCWD, path, 0);
}

int renameat(int olddirfd, const char *oldpath,
             int newdirfd, const char *newpath)
{
    return translate_zero_result(
        mini_sys_renameat(olddirfd, oldpath, newdirfd, newpath));
}

int rename(const char *oldpath, const char *newpath)
{
    return renameat(AT_FDCWD, oldpath, AT_FDCWD, newpath);
}

int remove(const char *path)
{
    int saved_errno = errno;
    long result;

    if (path == (const char *)0) {
        errno = EINVAL;
        return -1;
    }

    result = unlinkat_raw(AT_FDCWD, path, 0);
    if (result == -EISDIR) {
        result = unlinkat_raw(AT_FDCWD, path, AT_REMOVEDIR);
    }
    if (result < 0L) {
        errno = (int)-result;
        return -1;
    }

    errno = saved_errno;
    return 0;
}
