#include <errno.h>
#include <fcntl.h>
#include <mini/syscall.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int same_bytes(const char *left, const char *right, size_t n)
{
    size_t i;

    for (i = 0U; i < n; ++i) {
        if (left[i] != right[i]) {
            return 0;
        }
    }
    return 1;
}

static int read_exact_path(const char *path, const char *expected, size_t length)
{
    char buffer[16];
    int fd;
    ssize_t count;

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        return 0;
    }
    count = read(fd, buffer, length);
    if (count != (ssize_t)length || !same_bytes(buffer, expected, length) ||
        close(fd) != 0) {
        return 0;
    }
    return 1;
}

int main(int argc, char **argv)
{
    static const char ok[] = "posix-path-ok\n";
    char buffer[8];
    int dirfd;
    int fd;

    if (argc != 5) {
        return 1;
    }

    errno = ERANGE;
    fd = open(argv[1], O_CREAT | O_EXCL | O_WRONLY, 0600U);
    if (fd < 0 || errno != ERANGE ||
        write(fd, "root", 4U) != 4 || errno != ERANGE ||
        close(fd) != 0 || errno != ERANGE) {
        return 2;
    }

    errno = ERANGE;
    if (rename(argv[1], argv[2]) != 0 || errno != ERANGE) {
        return 3;
    }
    errno = ERANGE;
    if (open(argv[1], O_RDONLY) != -1 || errno != ENOENT) {
        return 4;
    }
    errno = ERANGE;
    if (!read_exact_path(argv[2], "root", 4U) || errno != ERANGE) {
        return 5;
    }
    errno = ERANGE;
    if (unlink(argv[2]) != 0 || errno != ERANGE) {
        return 6;
    }
    errno = ERANGE;
    if (unlink(argv[2]) != -1 || errno != ENOENT) {
        return 7;
    }

    errno = ERANGE;
    dirfd = open(argv[3], O_RDONLY);
    if (dirfd < 0 || errno != ERANGE) {
        return 8;
    }
    fd = openat(dirfd, "source", O_CREAT | O_EXCL | O_RDWR, 0600U);
    if (fd < 0 || write(fd, "inside", 6U) != 6 || close(fd) != 0) {
        close(dirfd);
        return 9;
    }

    errno = ERANGE;
    if (renameat(dirfd, "source", dirfd, "target") != 0 ||
        errno != ERANGE) {
        close(dirfd);
        return 10;
    }
    errno = ERANGE;
    if (openat(dirfd, "source", O_RDONLY) != -1 || errno != ENOENT) {
        close(dirfd);
        return 11;
    }

    errno = ERANGE;
    fd = openat(dirfd, "target", O_RDONLY);
    if (fd < 0 || errno != ERANGE ||
        read(fd, buffer, 6U) != 6 ||
        !same_bytes(buffer, "inside", 6U) ||
        close(fd) != 0) {
        close(dirfd);
        return 12;
    }

    errno = ERANGE;
    if (unlinkat(dirfd, "target", 0) != 0 || errno != ERANGE) {
        close(dirfd);
        return 13;
    }
    errno = ERANGE;
    if (unlinkat(dirfd, "target", 0) != -1 || errno != ENOENT) {
        close(dirfd);
        return 14;
    }
    errno = ERANGE;
    if (unlinkat(-1, "relative", 0) != -1 || errno != EBADF) {
        close(dirfd);
        return 15;
    }
    if (close(dirfd) != 0) {
        return 16;
    }

    errno = ERANGE;
    if (unlink(argv[3]) != -1 || errno != EISDIR) {
        return 17;
    }
    errno = ERANGE;
    if (unlinkat(AT_FDCWD, argv[3], AT_REMOVEDIR) != 0 ||
        errno != ERANGE) {
        return 18;
    }

    errno = ERANGE;
    if (remove(argv[4]) != 0 || errno != ERANGE) {
        return 19;
    }
    errno = ERANGE;
    if (remove(argv[4]) != -1 || errno != ENOENT) {
        return 20;
    }

    if (mini_sys_write(STDOUT_FILENO, ok, sizeof(ok) - 1U) !=
        (long)(sizeof(ok) - 1U)) {
        return 21;
    }
    return 0;
}
