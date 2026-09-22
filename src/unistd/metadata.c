#include <errno.h>
#include <fcntl.h>
#include <mini/syscall.h>
#include <sys/stat.h>
#include <unistd.h>

typedef char mini_stat_abi_size_check[(sizeof(struct stat) == 144U) ? 1 : -1];

static int translate_zero(long result)
{
    if (result < 0L) {
        errno = (int)-result;
        return -1;
    }
    return 0;
}

int fstat(int fd, struct stat *buf)
{
    return translate_zero(mini_sys_fstat(fd, buf));
}

int stat(const char *restrict path, struct stat *restrict buf)
{
    return translate_zero(mini_sys_newfstatat(AT_FDCWD, path, buf, 0));
}

int ftruncate(int fd, off_t length)
{
    return translate_zero(mini_sys_ftruncate(fd, (long)length));
}
