#include <errno.h>
#include <fcntl.h>
#include <mini/syscall.h>
#include <signal.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

static int same_limit(const struct rlimit *left, const struct rlimit *right)
{
    return left->rlim_cur == right->rlim_cur &&
           left->rlim_max == right->rlim_max;
}

static int child_limit_file(const char *path, const struct rlimit *inherited)
{
    struct rlimit current;
    struct rlimit limited;
    struct rlimit invalid;
    struct stat info;
    char byte;
    int fd;

    errno = ERANGE;
    if (getrlimit(RLIMIT_FSIZE, &current) != 0 ||
        errno != ERANGE ||
        !same_limit(&current, inherited) ||
        current.rlim_max < 4UL) {
        return 20;
    }

    limited.rlim_cur = 4UL;
    limited.rlim_max = current.rlim_max;
    errno = ERANGE;
    if (setrlimit(RLIMIT_FSIZE, &limited) != 0 ||
        errno != ERANGE ||
        getrlimit(RLIMIT_FSIZE, &current) != 0 ||
        errno != ERANGE ||
        current.rlim_cur != 4UL ||
        current.rlim_max != limited.rlim_max) {
        return 21;
    }

    invalid.rlim_cur = 5UL;
    invalid.rlim_max = 4UL;
    errno = ERANGE;
    if (setrlimit(RLIMIT_FSIZE, &invalid) != -1 ||
        errno != EINVAL) {
        return 22;
    }

    errno = ERANGE;
    if (signal(SIGXFSZ, SIG_IGN) == SIG_ERR || errno != ERANGE) {
        return 23;
    }

    errno = ERANGE;
    fd = open(path, O_CREAT | O_EXCL | O_RDWR, 0600U);
    if (fd < 0 || errno != ERANGE) {
        return 24;
    }

    errno = ERANGE;
    if (write(fd, "ABCD", 4U) != 4 || errno != ERANGE) {
        close(fd);
        return 25;
    }

    errno = ERANGE;
    if (write(fd, "E", 1U) != (ssize_t)-1 || errno != EFBIG) {
        close(fd);
        return 26;
    }

    errno = ERANGE;
    if (fstat(fd, &info) != 0 || errno != ERANGE ||
        info.st_size != (off_t)4) {
        close(fd);
        return 27;
    }

    errno = ERANGE;
    if (lseek(fd, 0, SEEK_SET) != 0 || errno != ERANGE ||
        read(fd, &byte, 1U) != 1 || errno != ERANGE ||
        byte != 'A') {
        close(fd);
        return 28;
    }

    errno = ERANGE;
    if (close(fd) != 0 || errno != ERANGE) {
        return 29;
    }

    return 0;
}

int main(int argc, char **argv)
{
    static const char ok[] = "resource-limit-ok\n";
    struct rlimit before;
    struct rlimit after;
    struct stat info;
    char buffer[4];
    int fd;
    int status;
    pid_t child;

    if (argc != 2) {
        return 1;
    }

    errno = ERANGE;
    if (getrlimit(RLIMIT_FSIZE, &before) != 0 ||
        errno != ERANGE ||
        before.rlim_max < 4UL) {
        return 2;
    }

    errno = ERANGE;
    if (getrlimit(-1, &after) != -1 || errno != EINVAL) {
        return 3;
    }

    errno = ERANGE;
    child = fork();
    if (child < 0) {
        return 4;
    }
    if (child == 0) {
        _Exit(child_limit_file(argv[1], &before));
    }

    status = -1;
    errno = ERANGE;
    if (waitpid(child, &status, 0) != child ||
        errno != ERANGE ||
        !WIFEXITED(status) ||
        WEXITSTATUS(status) != 0) {
        (void)unlink(argv[1]);
        return 5;
    }

    errno = ERANGE;
    if (getrlimit(RLIMIT_FSIZE, &after) != 0 ||
        errno != ERANGE ||
        !same_limit(&before, &after)) {
        (void)unlink(argv[1]);
        return 6;
    }

    errno = ERANGE;
    if (stat(argv[1], &info) != 0 ||
        errno != ERANGE ||
        info.st_size != (off_t)4) {
        (void)unlink(argv[1]);
        return 7;
    }

    errno = ERANGE;
    fd = open(argv[1], O_RDONLY);
    if (fd < 0 || errno != ERANGE ||
        read(fd, buffer, sizeof(buffer)) != (ssize_t)sizeof(buffer) ||
        errno != ERANGE ||
        memcmp(buffer, "ABCD", sizeof(buffer)) != 0 ||
        close(fd) != 0 ||
        errno != ERANGE) {
        if (fd >= 0) {
            (void)close(fd);
        }
        (void)unlink(argv[1]);
        return 8;
    }

    errno = ERANGE;
    if (unlink(argv[1]) != 0 || errno != ERANGE) {
        return 9;
    }

    if (mini_sys_write(STDOUT_FILENO, ok, sizeof(ok) - 1U) !=
        (long)(sizeof(ok) - 1U)) {
        return 10;
    }
    return 0;
}
