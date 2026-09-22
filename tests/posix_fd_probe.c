#include <errno.h>
#include <fcntl.h>
#include <mini/syscall.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

static int cleanup(const char *path)
{
    errno = ERANGE;
    if (remove(path) == 0) {
        return errno == ERANGE;
    }
    return errno == ENOENT;
}

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

int main(int argc, char **argv)
{
    static const char ok[] = "posix-fd-ok\n";
    static const char base[] = "abcdef";
    static const char tail[] = "XYZ";
    char buffer[10];
    mode_t mode = 0600U;
    int fd;
    ssize_t count;

    if (argc != 2 || !cleanup(argv[1])) {
        return 1;
    }

    errno = ERANGE;
    fd = open(argv[1], O_CREAT | O_EXCL | O_RDWR, mode);
    if (fd < 0 || errno != ERANGE) {
        cleanup(argv[1]);
        return 2;
    }

    count = write(fd, base, sizeof(base) - 1U);
    if (count != (ssize_t)(sizeof(base) - 1U) || errno != ERANGE) {
        close(fd);
        cleanup(argv[1]);
        return 3;
    }
    if (lseek(fd, 2, SEEK_SET) != 2 || errno != ERANGE) {
        close(fd);
        cleanup(argv[1]);
        return 4;
    }
    if (write(fd, "12", 2U) != 2 || errno != ERANGE ||
        lseek(fd, 0, SEEK_SET) != 0 || errno != ERANGE) {
        close(fd);
        cleanup(argv[1]);
        return 5;
    }

    count = read(fd, buffer, 6U);
    if (count != 6 || !same_bytes(buffer, "ab12ef", 6U) ||
        errno != ERANGE) {
        close(fd);
        cleanup(argv[1]);
        return 6;
    }
    if (close(fd) != 0 || errno != ERANGE) {
        cleanup(argv[1]);
        return 7;
    }

    errno = ERANGE;
    if (open(argv[1], O_CREAT | O_EXCL | O_WRONLY, mode) != -1 ||
        errno != EEXIST) {
        cleanup(argv[1]);
        return 8;
    }

    errno = ERANGE;
    fd = openat(AT_FDCWD, argv[1], O_WRONLY | O_APPEND);
    if (fd < 0 || errno != ERANGE) {
        cleanup(argv[1]);
        return 9;
    }
    if (write(fd, tail, sizeof(tail) - 1U) !=
            (ssize_t)(sizeof(tail) - 1U) ||
        errno != ERANGE || close(fd) != 0 || errno != ERANGE) {
        cleanup(argv[1]);
        return 10;
    }

    errno = ERANGE;
    fd = open(argv[1], O_RDONLY);
    if (fd < 0 || errno != ERANGE) {
        cleanup(argv[1]);
        return 11;
    }
    count = read(fd, buffer, 9U);
    if (count != 9 || !same_bytes(buffer, "ab12efXYZ", 9U) ||
        errno != ERANGE || close(fd) != 0 || errno != ERANGE) {
        cleanup(argv[1]);
        return 12;
    }

    errno = ERANGE;
    if (read(-1, buffer, 1U) != (ssize_t)-1 || errno != EBADF) {
        cleanup(argv[1]);
        return 13;
    }
    errno = ERANGE;
    if (write(-1, buffer, 1U) != (ssize_t)-1 || errno != EBADF) {
        cleanup(argv[1]);
        return 14;
    }
    errno = ERANGE;
    if (close(-1) != -1 || errno != EBADF) {
        cleanup(argv[1]);
        return 15;
    }
    errno = ERANGE;
    if (lseek(-1, 0, SEEK_SET) != (off_t)-1 || errno != EBADF) {
        cleanup(argv[1]);
        return 16;
    }

    errno = ERANGE;
    if (open("build/mini-libc-posix-fd-definitely-missing", O_RDONLY) != -1 ||
        errno != ENOENT) {
        cleanup(argv[1]);
        return 17;
    }

    errno = ERANGE;
    fd = open(argv[1], O_RDONLY);
    if (fd < 0 || errno != ERANGE) {
        cleanup(argv[1]);
        return 18;
    }
    if (lseek(fd, 0, 99) != (off_t)-1 || errno != EINVAL) {
        close(fd);
        cleanup(argv[1]);
        return 19;
    }
    if (close(fd) != 0) {
        cleanup(argv[1]);
        return 20;
    }

    if (!cleanup(argv[1])) {
        return 21;
    }
    if (mini_sys_write(STDOUT_FILENO, ok, sizeof(ok) - 1U) !=
        (long)(sizeof(ok) - 1U)) {
        return 22;
    }
    return 0;
}
