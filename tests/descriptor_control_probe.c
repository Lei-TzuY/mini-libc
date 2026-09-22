#include <errno.h>
#include <fcntl.h>
#include <mini/syscall.h>
#include <stddef.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int same_bytes(const char *left, const char *right, size_t count)
{
    size_t index;

    for (index = 0U; index < count; ++index) {
        if (left[index] != right[index]) {
            return 0;
        }
    }
    return 1;
}

static int write_file(const char *path, const char *data, size_t count)
{
    int fd = open(path, O_CREAT | O_EXCL | O_WRONLY, 0600U);

    if (fd < 0) {
        return 0;
    }
    if (write(fd, data, count) != (ssize_t)count) {
        (void)close(fd);
        return 0;
    }
    return close(fd) == 0;
}

int main(int argc, char **argv)
{
    static const char ok[] = "descriptor-control-ok\n";
    char buffer[4];
    struct stat source_stat;
    struct stat replacement_before;
    struct stat replacement_after;
    int fd;
    int duplicate;
    int high_duplicate;
    int replacement;
    int flags;

    if (argc != 3) {
        return 1;
    }

    errno = ERANGE;
    fd = open(argv[1], O_CREAT | O_EXCL | O_RDWR, 0600U);
    if (fd < 0 || errno != ERANGE ||
        write(fd, "abcdef", 6U) != 6 || errno != ERANGE ||
        lseek(fd, 0, SEEK_SET) != 0 || errno != ERANGE) {
        return 2;
    }

    errno = ERANGE;
    duplicate = dup(fd);
    if (duplicate < 0 || duplicate == fd || errno != ERANGE) {
        close(fd);
        unlink(argv[1]);
        return 3;
    }

    if (read(fd, buffer, 2U) != 2 ||
        !same_bytes(buffer, "ab", 2U) ||
        read(duplicate, buffer, 2U) != 2 ||
        !same_bytes(buffer, "cd", 2U)) {
        close(duplicate);
        close(fd);
        unlink(argv[1]);
        return 4;
    }

    errno = ERANGE;
    flags = fcntl(fd, F_GETFL);
    if (flags < 0 || (flags & O_ACCMODE) != O_RDWR || errno != ERANGE ||
        fcntl(duplicate, F_GETFL) != flags || errno != ERANGE) {
        close(duplicate);
        close(fd);
        unlink(argv[1]);
        return 5;
    }

    errno = ERANGE;
    if (fcntl(fd, F_GETFD) != 0 || errno != ERANGE ||
        fcntl(duplicate, F_GETFD) != 0 || errno != ERANGE ||
        fcntl(duplicate, F_SETFD, FD_CLOEXEC) != 0 || errno != ERANGE ||
        fcntl(duplicate, F_GETFD) != FD_CLOEXEC || errno != ERANGE ||
        fcntl(fd, F_GETFD) != 0 || errno != ERANGE) {
        close(duplicate);
        close(fd);
        unlink(argv[1]);
        return 6;
    }

    errno = ERANGE;
    high_duplicate = fcntl(duplicate, F_DUPFD, 100);
    if (high_duplicate < 100 || errno != ERANGE ||
        fcntl(high_duplicate, F_GETFD) != 0 || errno != ERANGE) {
        if (high_duplicate >= 0) {
            close(high_duplicate);
        }
        close(duplicate);
        close(fd);
        unlink(argv[1]);
        return 7;
    }

    if (read(high_duplicate, buffer, 1U) != 1 || buffer[0] != 'e' ||
        read(fd, buffer, 1U) != 1 || buffer[0] != 'f') {
        close(high_duplicate);
        close(duplicate);
        close(fd);
        unlink(argv[1]);
        return 8;
    }

    if (!write_file(argv[2], "ZZ", 2U)) {
        close(high_duplicate);
        close(duplicate);
        close(fd);
        unlink(argv[1]);
        return 9;
    }

    replacement = open(argv[2], O_RDONLY);
    if (replacement < 0 ||
        fstat(replacement, &replacement_before) != 0 ||
        fstat(fd, &source_stat) != 0) {
        if (replacement >= 0) {
            close(replacement);
        }
        close(high_duplicate);
        close(duplicate);
        close(fd);
        unlink(argv[1]);
        unlink(argv[2]);
        return 10;
    }

    errno = ERANGE;
    if (dup2(fd, replacement) != replacement || errno != ERANGE ||
        fstat(replacement, &replacement_after) != 0 || errno != ERANGE ||
        replacement_after.st_dev != source_stat.st_dev ||
        replacement_after.st_ino != source_stat.st_ino ||
        (replacement_before.st_dev == replacement_after.st_dev &&
         replacement_before.st_ino == replacement_after.st_ino)) {
        close(replacement);
        close(high_duplicate);
        close(duplicate);
        close(fd);
        unlink(argv[1]);
        unlink(argv[2]);
        return 11;
    }

    errno = ERANGE;
    if (dup2(fd, fd) != fd || errno != ERANGE) {
        close(replacement);
        close(high_duplicate);
        close(duplicate);
        close(fd);
        unlink(argv[1]);
        unlink(argv[2]);
        return 12;
    }

    errno = ERANGE;
    if (dup(-1) != -1 || errno != EBADF ||
        dup2(-1, replacement) != -1 || errno != EBADF ||
        fcntl(-1, F_GETFD) != -1 || errno != EBADF) {
        close(replacement);
        close(high_duplicate);
        close(duplicate);
        close(fd);
        unlink(argv[1]);
        unlink(argv[2]);
        return 13;
    }

    errno = ERANGE;
    if (fcntl(fd, 9999) != -1 || errno != EINVAL) {
        close(replacement);
        close(high_duplicate);
        close(duplicate);
        close(fd);
        unlink(argv[1]);
        unlink(argv[2]);
        return 14;
    }

    if (close(replacement) != 0 ||
        close(high_duplicate) != 0 ||
        close(duplicate) != 0 ||
        close(fd) != 0) {
        unlink(argv[1]);
        unlink(argv[2]);
        return 15;
    }

    errno = ERANGE;
    replacement = open(argv[2], O_RDONLY);
    if (replacement < 0 || errno != ERANGE ||
        read(replacement, buffer, 2U) != 2 ||
        !same_bytes(buffer, "ZZ", 2U) ||
        close(replacement) != 0) {
        unlink(argv[1]);
        unlink(argv[2]);
        return 16;
    }

    errno = ERANGE;
    if (unlink(argv[1]) != 0 || errno != ERANGE ||
        unlink(argv[2]) != 0 || errno != ERANGE) {
        return 17;
    }

    if (mini_sys_write(STDOUT_FILENO, ok, sizeof(ok) - 1U) !=
        (long)(sizeof(ok) - 1U)) {
        return 18;
    }
    return 0;
}
