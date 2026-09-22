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

static int same_identity(const struct stat *left, const struct stat *right)
{
    return left->st_dev == right->st_dev && left->st_ino == right->st_ino;
}

static int write_relative_file(const char *name, const char *data, size_t count)
{
    int fd = open(name, O_CREAT | O_EXCL | O_WRONLY, 0600U);

    if (fd < 0) {
        return 0;
    }
    if (write(fd, data, count) != (ssize_t)count) {
        (void)close(fd);
        return 0;
    }
    return close(fd) == 0;
}

static int read_relative_file(const char *name, const char *expected, size_t count)
{
    char buffer[16];
    int fd = open(name, O_RDONLY);

    if (fd < 0 || count > sizeof(buffer)) {
        if (fd >= 0) {
            (void)close(fd);
        }
        return 0;
    }
    if (read(fd, buffer, count) != (ssize_t)count ||
        close(fd) != 0) {
        return 0;
    }
    return memcmp(buffer, expected, count) == 0;
}

static int directory_has(DIR *directory, const char *name)
{
    struct dirent *entry;

    for (;;) {
        entry = readdir(directory);
        if (entry == (struct dirent *)0) {
            return 0;
        }
        if (same_string(entry->d_name, name)) {
            return 1;
        }
    }
}

int main(int argc, char **argv)
{
    static const char ok[] = "cwd-state-ok\n";
    char original[1024];
    char current[1024];
    char tiny[1];
    struct stat root_before;
    struct stat root_after;
    struct stat child_before;
    struct stat child_after;
    struct stat saved_stat;
    DIR *directory;
    int saved_fd;

    if (argc != 2) {
        return 1;
    }

    errno = ERANGE;
    if (getcwd(original, sizeof(original)) != original || errno != ERANGE) {
        return 2;
    }

    errno = ERANGE;
    if (getcwd(tiny, sizeof(tiny)) != (char *)0 || errno != ERANGE) {
        return 3;
    }

    errno = ERANGE;
    if (getcwd((char *)0, sizeof(current)) != (char *)0 || errno != EINVAL ||
        getcwd(current, 0U) != (char *)0 || errno != EINVAL) {
        return 4;
    }

    errno = ERANGE;
    saved_fd = open(".", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (saved_fd < 0 || errno != ERANGE ||
        fstat(saved_fd, &saved_stat) != 0 || errno != ERANGE ||
        !S_ISDIR(saved_stat.st_mode)) {
        if (saved_fd >= 0) {
            close(saved_fd);
        }
        return 5;
    }

    errno = ERANGE;
    if (stat(argv[1], &root_before) != 0 || errno != ERANGE ||
        !S_ISDIR(root_before.st_mode)) {
        close(saved_fd);
        return 6;
    }

    errno = ERANGE;
    if (chdir(argv[1]) != 0 || errno != ERANGE ||
        stat(".", &root_after) != 0 || errno != ERANGE ||
        !same_identity(&root_before, &root_after)) {
        fchdir(saved_fd);
        close(saved_fd);
        return 7;
    }

    errno = ERANGE;
    if (getcwd(current, sizeof(current)) != current || errno != ERANGE ||
        same_string(current, original)) {
        fchdir(saved_fd);
        close(saved_fd);
        return 8;
    }

    if (!write_relative_file("marker", "ROOT", 4U) ||
        !read_relative_file("./marker", "ROOT", 4U)) {
        fchdir(saved_fd);
        close(saved_fd);
        return 9;
    }

    errno = ERANGE;
    directory = opendir(".");
    if (directory == (DIR *)0 || errno != ERANGE ||
        !directory_has(directory, "marker") ||
        closedir(directory) != 0 || errno != ERANGE) {
        if (directory != (DIR *)0) {
            (void)closedir(directory);
        }
        (void)unlink("marker");
        fchdir(saved_fd);
        close(saved_fd);
        return 10;
    }

    errno = ERANGE;
    if (stat("child", &child_before) != 0 || errno != ERANGE ||
        !S_ISDIR(child_before.st_mode) ||
        chdir("child") != 0 || errno != ERANGE ||
        stat(".", &child_after) != 0 || errno != ERANGE ||
        !same_identity(&child_before, &child_after)) {
        (void)chdir("..");
        (void)unlink("marker");
        fchdir(saved_fd);
        close(saved_fd);
        return 11;
    }

    errno = ERANGE;
    if (!read_relative_file("../marker", "ROOT", 4U) || errno != ERANGE ||
        !write_relative_file("inside", "CHILD", 5U)) {
        (void)unlink("inside");
        (void)chdir("..");
        (void)unlink("marker");
        fchdir(saved_fd);
        close(saved_fd);
        return 12;
    }

    errno = ERANGE;
    directory = opendir("..");
    if (directory == (DIR *)0 || errno != ERANGE ||
        !directory_has(directory, "marker") ||
        closedir(directory) != 0 || errno != ERANGE) {
        if (directory != (DIR *)0) {
            (void)closedir(directory);
        }
        (void)unlink("inside");
        (void)chdir("..");
        (void)unlink("marker");
        fchdir(saved_fd);
        close(saved_fd);
        return 13;
    }

    errno = ERANGE;
    if (chdir("mini-libc-cwd-definitely-missing") != -1 || errno != ENOENT ||
        stat(".", &child_after) != 0 ||
        !same_identity(&child_before, &child_after)) {
        (void)unlink("inside");
        (void)chdir("..");
        (void)unlink("marker");
        fchdir(saved_fd);
        close(saved_fd);
        return 14;
    }

    errno = ERANGE;
    if (chdir("inside") != -1 || errno != ENOTDIR) {
        (void)unlink("inside");
        (void)chdir("..");
        (void)unlink("marker");
        fchdir(saved_fd);
        close(saved_fd);
        return 15;
    }

    errno = ERANGE;
    if (fchdir(-1) != -1 || errno != EBADF) {
        (void)unlink("inside");
        (void)chdir("..");
        (void)unlink("marker");
        fchdir(saved_fd);
        close(saved_fd);
        return 16;
    }

    errno = ERANGE;
    if (unlink("inside") != 0 || errno != ERANGE ||
        chdir("..") != 0 || errno != ERANGE ||
        unlink("marker") != 0 || errno != ERANGE) {
        fchdir(saved_fd);
        close(saved_fd);
        return 17;
    }

    errno = ERANGE;
    if (fchdir(saved_fd) != 0 || errno != ERANGE ||
        getcwd(current, sizeof(current)) != current || errno != ERANGE ||
        !same_string(current, original) ||
        stat(".", &root_after) != 0 || errno != ERANGE ||
        !same_identity(&root_after, &saved_stat)) {
        close(saved_fd);
        return 18;
    }

    if (close(saved_fd) != 0) {
        return 19;
    }

    if (mini_sys_write(STDOUT_FILENO, ok, sizeof(ok) - 1U) !=
        (long)(sizeof(ok) - 1U)) {
        return 20;
    }
    return 0;
}
