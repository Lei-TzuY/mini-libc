#include <errno.h>
#include <fcntl.h>
#include <mini/syscall.h>
#include <stddef.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int same_bytes(const unsigned char *left,
                      const unsigned char *right, size_t n)
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
    static const unsigned char ten[] = "abcdefghij";
    static const unsigned char four[] = "abcd";
    static const unsigned char grown[] = {
        'a', 'b', 'c', 'd', 0U, 0U, 0U, 0U
    };
    static const char ok[] = "metadata-ok\n";
    struct stat by_fd;
    struct stat by_path;
    struct stat dir_stat;
    unsigned char buffer[12];
    int fd;
    int dirfd;

    if (argc != 3 || sizeof(struct stat) != 144U) {
        return 1;
    }

    errno = ERANGE;
    fd = open(argv[1], O_CREAT | O_EXCL | O_RDWR, 0600U);
    if (fd < 0 || errno != ERANGE) {
        return 2;
    }

    if (write(fd, ten, 10U) != 10 || errno != ERANGE) {
        close(fd);
        unlink(argv[1]);
        return 3;
    }

    errno = ERANGE;
    if (fstat(fd, &by_fd) != 0 || errno != ERANGE ||
        !S_ISREG(by_fd.st_mode) || by_fd.st_size != 10 ||
        by_fd.st_nlink < 1U || by_fd.st_blksize <= 0) {
        close(fd);
        unlink(argv[1]);
        return 4;
    }

    errno = ERANGE;
    if (stat(argv[1], &by_path) != 0 || errno != ERANGE ||
        !S_ISREG(by_path.st_mode) || by_path.st_size != 10 ||
        by_path.st_dev != by_fd.st_dev || by_path.st_ino != by_fd.st_ino) {
        close(fd);
        unlink(argv[1]);
        return 5;
    }

    errno = ERANGE;
    if (ftruncate(fd, 4) != 0 || errno != ERANGE ||
        fstat(fd, &by_fd) != 0 || errno != ERANGE ||
        by_fd.st_size != 4) {
        close(fd);
        unlink(argv[1]);
        return 6;
    }
    if (lseek(fd, 0, SEEK_SET) != 0 ||
        read(fd, buffer, 4U) != 4 ||
        !same_bytes(buffer, four, 4U)) {
        close(fd);
        unlink(argv[1]);
        return 7;
    }

    errno = ERANGE;
    if (ftruncate(fd, 8) != 0 || errno != ERANGE ||
        fstat(fd, &by_fd) != 0 || errno != ERANGE ||
        by_fd.st_size != 8) {
        close(fd);
        unlink(argv[1]);
        return 8;
    }
    if (lseek(fd, 0, SEEK_SET) != 0 ||
        read(fd, buffer, 8U) != 8 ||
        !same_bytes(buffer, grown, 8U)) {
        close(fd);
        unlink(argv[1]);
        return 9;
    }

    errno = ERANGE;
    if (stat(argv[1], &by_path) != 0 || errno != ERANGE ||
        by_path.st_size != 8 || !S_ISREG(by_path.st_mode)) {
        close(fd);
        unlink(argv[1]);
        return 10;
    }

    errno = ERANGE;
    if (ftruncate(fd, (off_t)-1) != -1 || errno != EINVAL) {
        close(fd);
        unlink(argv[1]);
        return 11;
    }

    if (close(fd) != 0) {
        unlink(argv[1]);
        return 12;
    }

    errno = ERANGE;
    if (fstat(-1, &by_fd) != -1 || errno != EBADF) {
        unlink(argv[1]);
        return 13;
    }
    errno = ERANGE;
    if (ftruncate(-1, 0) != -1 || errno != EBADF) {
        unlink(argv[1]);
        return 14;
    }

    errno = ERANGE;
    if (stat("build/mini-libc-metadata-definitely-missing", &by_path) != -1 ||
        errno != ENOENT) {
        unlink(argv[1]);
        return 15;
    }

    errno = ERANGE;
    if (stat(argv[2], &dir_stat) != 0 || errno != ERANGE ||
        !S_ISDIR(dir_stat.st_mode) || dir_stat.st_size < 0) {
        unlink(argv[1]);
        return 16;
    }

    errno = ERANGE;
    dirfd = open(argv[2], O_RDONLY);
    if (dirfd < 0 || errno != ERANGE) {
        unlink(argv[1]);
        return 17;
    }
    errno = ERANGE;
    if (fstat(dirfd, &by_fd) != 0 || errno != ERANGE ||
        !S_ISDIR(by_fd.st_mode) ||
        by_fd.st_dev != dir_stat.st_dev || by_fd.st_ino != dir_stat.st_ino) {
        close(dirfd);
        unlink(argv[1]);
        return 18;
    }
    if (close(dirfd) != 0) {
        unlink(argv[1]);
        return 19;
    }

    errno = ERANGE;
    if (unlink(argv[1]) != 0 || errno != ERANGE) {
        return 20;
    }

    if (mini_sys_write(STDOUT_FILENO, ok, sizeof(ok) - 1U) !=
        (long)(sizeof(ok) - 1U)) {
        return 21;
    }
    return 0;
}
