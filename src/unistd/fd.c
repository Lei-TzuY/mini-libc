#include <errno.h>
#include <fcntl.h>
#include <mini/syscall.h>
#include <stdarg.h>
#include <stddef.h>
#include <sys/types.h>
#include <unistd.h>

static int syscall_failed(long result)
{
    return result < 0L;
}

static int open_needs_mode(int flags)
{
    return (flags & O_CREAT) != 0;
}

static int openat_impl(int dirfd, const char *path, int flags, mode_t mode)
{
    long result = mini_sys_openat(dirfd, path, flags, mode);

    if (syscall_failed(result)) {
        errno = (int)-result;
        return -1;
    }
    return (int)result;
}

ssize_t read(int fd, void *buf, size_t count)
{
    long result = mini_sys_read(fd, buf, (unsigned long)count);

    if (syscall_failed(result)) {
        errno = (int)-result;
        return (ssize_t)-1;
    }
    return (ssize_t)result;
}

ssize_t write(int fd, const void *buf, size_t count)
{
    long result = mini_sys_write(fd, buf, (unsigned long)count);

    if (syscall_failed(result)) {
        errno = (int)-result;
        return (ssize_t)-1;
    }
    return (ssize_t)result;
}

int close(int fd)
{
    long result = mini_sys_close(fd);

    if (syscall_failed(result)) {
        errno = (int)-result;
        return -1;
    }
    return 0;
}

off_t lseek(int fd, off_t offset, int whence)
{
    long result = mini_sys_lseek(fd, (long)offset, whence);

    if (syscall_failed(result)) {
        errno = (int)-result;
        return (off_t)-1;
    }
    return (off_t)result;
}

int openat(int dirfd, const char *path, int flags, ...)
{
    mode_t mode = 0U;

    if (open_needs_mode(flags)) {
        va_list args;

        va_start(args, flags);
        mode = va_arg(args, mode_t);
        va_end(args);
    }
    return openat_impl(dirfd, path, flags, mode);
}

int open(const char *path, int flags, ...)
{
    mode_t mode = 0U;

    if (open_needs_mode(flags)) {
        va_list args;

        va_start(args, flags);
        mode = va_arg(args, mode_t);
        va_end(args);
    }
    return openat_impl(AT_FDCWD, path, flags, mode);
}
