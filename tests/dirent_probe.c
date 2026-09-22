#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <mini/syscall.h>
#include <stddef.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int same_string(const char *left, const char *right)
{
    while (*left != '\0' && *left == *right) {
        ++left;
        ++right;
    }
    return *left == *right;
}

static int create_file_at(int dirfd_value, const char *name,
                          const char *payload)
{
    int fd;
    size_t length = 0U;

    while (payload[length] != '\0') {
        ++length;
    }

    fd = openat(dirfd_value, name, O_CREAT | O_EXCL | O_WRONLY, 0600U);
    if (fd < 0) {
        return 0;
    }
    if (write(fd, payload, length) != (ssize_t)length) {
        (void)close(fd);
        return 0;
    }
    return close(fd) == 0;
}

static int scan_directory(DIR *directory,
                          int *dot_seen, int *dotdot_seen,
                          int *alpha_seen, int *beta_seen,
                          int *subdir_seen)
{
    struct dirent *entry;

    *dot_seen = 0;
    *dotdot_seen = 0;
    *alpha_seen = 0;
    *beta_seen = 0;
    *subdir_seen = 0;

    for (;;) {
        entry = readdir(directory);
        if (entry == (struct dirent *)0) {
            return errno == ERANGE;
        }

        if (entry->d_ino == 0U || entry->d_reclen < 20U ||
            entry->d_name[0] == '\0') {
            return 0;
        }

        if (same_string(entry->d_name, ".")) {
            *dot_seen = 1;
            if (entry->d_type != DT_DIR && entry->d_type != DT_UNKNOWN) {
                return 0;
            }
        } else if (same_string(entry->d_name, "..")) {
            *dotdot_seen = 1;
        } else if (same_string(entry->d_name, "alpha")) {
            *alpha_seen = 1;
            if (entry->d_type != DT_REG && entry->d_type != DT_UNKNOWN) {
                return 0;
            }
        } else if (same_string(entry->d_name, "beta")) {
            *beta_seen = 1;
            if (entry->d_type != DT_REG && entry->d_type != DT_UNKNOWN) {
                return 0;
            }
        } else if (same_string(entry->d_name, "subdir")) {
            *subdir_seen = 1;
            if (entry->d_type != DT_DIR && entry->d_type != DT_UNKNOWN) {
                return 0;
            }
        }
    }
}

int main(int argc, char **argv)
{
    static const char ok[] = "dirent-ok\n";
    struct stat path_stat;
    struct stat fd_stat;
    DIR *directory;
    int rootfd;
    int dot_seen;
    int dotdot_seen;
    int alpha_seen;
    int beta_seen;
    int subdir_seen;

    if (argc != 2) {
        return 1;
    }

    errno = ERANGE;
    rootfd = open(argv[1], O_RDONLY | O_DIRECTORY);
    if (rootfd < 0 || errno != ERANGE) {
        return 2;
    }

    if (!create_file_at(rootfd, "alpha", "A") ||
        !create_file_at(rootfd, "beta", "B")) {
        close(rootfd);
        return 3;
    }

    errno = ERANGE;
    if (mini_sys_mkdirat(rootfd, "subdir", 0700U) != 0L ||
        errno != ERANGE) {
        close(rootfd);
        return 4;
    }

    if (close(rootfd) != 0) {
        return 5;
    }

    errno = ERANGE;
    directory = opendir(argv[1]);
    if (directory == (DIR *)0 || errno != ERANGE) {
        return 6;
    }

    errno = ERANGE;
    if (dirfd(directory) < 0 || errno != ERANGE ||
        fstat(dirfd(directory), &fd_stat) != 0 ||
        stat(argv[1], &path_stat) != 0 ||
        !S_ISDIR(fd_stat.st_mode) ||
        fd_stat.st_dev != path_stat.st_dev ||
        fd_stat.st_ino != path_stat.st_ino) {
        closedir(directory);
        return 7;
    }

    errno = ERANGE;
    if (!scan_directory(directory, &dot_seen, &dotdot_seen,
                        &alpha_seen, &beta_seen, &subdir_seen) ||
        !dot_seen || !dotdot_seen || !alpha_seen ||
        !beta_seen || !subdir_seen) {
        closedir(directory);
        return 8;
    }

    errno = ERANGE;
    rewinddir(directory);
    if (errno != ERANGE ||
        !scan_directory(directory, &dot_seen, &dotdot_seen,
                        &alpha_seen, &beta_seen, &subdir_seen) ||
        !dot_seen || !dotdot_seen || !alpha_seen ||
        !beta_seen || !subdir_seen) {
        closedir(directory);
        return 9;
    }

    errno = ERANGE;
    if (closedir(directory) != 0 || errno != ERANGE) {
        return 10;
    }

    errno = ERANGE;
    directory = opendir("build/mini-libc-dirent-definitely-missing");
    if (directory != (DIR *)0 || errno != ENOENT) {
        return 11;
    }

    errno = ERANGE;
    directory = opendir("tests/dirent_probe.c");
    if (directory != (DIR *)0 || errno != ENOTDIR) {
        return 12;
    }

    rootfd = open(argv[1], O_RDONLY | O_DIRECTORY);
    if (rootfd < 0) {
        return 13;
    }
    if (unlinkat(rootfd, "alpha", 0) != 0 ||
        unlinkat(rootfd, "beta", 0) != 0 ||
        unlinkat(rootfd, "subdir", AT_REMOVEDIR) != 0 ||
        close(rootfd) != 0) {
        return 14;
    }

    if (mini_sys_write(STDOUT_FILENO, ok, sizeof(ok) - 1U) !=
        (long)(sizeof(ok) - 1U)) {
        return 15;
    }
    return 0;
}
