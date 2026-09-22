#include <errno.h>
#include <fcntl.h>
#include <mini/syscall.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

struct credential_snapshot {
    uid_t ruid;
    uid_t euid;
    gid_t rgid;
    gid_t egid;
};

static struct credential_snapshot snapshot(void)
{
    struct credential_snapshot result;

    result.ruid = getuid();
    result.euid = geteuid();
    result.rgid = getgid();
    result.egid = getegid();
    return result;
}

static int same_snapshot(const struct credential_snapshot *left,
                         const struct credential_snapshot *right)
{
    return left->ruid == right->ruid &&
           left->euid == right->euid &&
           left->rgid == right->rgid &&
           left->egid == right->egid;
}

static int read_exact(int fd, void *buffer, size_t count)
{
    unsigned char *bytes = (unsigned char *)buffer;
    size_t done = 0U;

    while (done < count) {
        ssize_t got = read(fd, bytes + done, count - done);

        if (got <= 0) {
            return 0;
        }
        done += (size_t)got;
    }
    return 1;
}

int main(int argc, char **argv)
{
    static const char ok[] = "credential-policy-ok\n";
    struct credential_snapshot parent_before;
    struct credential_snapshot parent_after;
    struct credential_snapshot inherited;
    struct credential_snapshot dropped;
    struct credential_snapshot executed;
    struct stat parent_file;
    struct stat child_file;
    char *child_argv[3];
    char *empty_envp[1];
    int report[2];
    int fd;
    int status;
    pid_t child;

    if (argc != 4) {
        return 1;
    }

    errno = ERANGE;
    parent_before = snapshot();
    if (errno != ERANGE) {
        return 2;
    }

    errno = ERANGE;
    fd = open(argv[2], O_CREAT | O_EXCL | O_RDWR, 0600U);
    if (fd < 0 || errno != ERANGE ||
        fstat(fd, &parent_file) != 0 || errno != ERANGE ||
        parent_file.st_uid != parent_before.euid ||
        parent_file.st_gid != parent_before.egid ||
        close(fd) != 0 || errno != ERANGE) {
        if (fd >= 0) {
            (void)close(fd);
        }
        (void)unlink(argv[2]);
        return 3;
    }

    errno = ERANGE;
    if (pipe(report) != 0 || errno != ERANGE) {
        (void)unlink(argv[2]);
        return 4;
    }

    errno = ERANGE;
    child = fork();
    if (child < 0) {
        close(report[0]);
        close(report[1]);
        unlink(argv[2]);
        return 5;
    }

    if (child == 0) {
        if (close(report[0]) != 0) {
            _Exit(90);
        }

        errno = ERANGE;
        inherited = snapshot();
        if (errno != ERANGE ||
            !same_snapshot(&inherited, &parent_before)) {
            _Exit(91);
        }

        errno = ERANGE;
        if (setgid(inherited.rgid) != 0 || errno != ERANGE ||
            setuid(inherited.ruid) != 0 || errno != ERANGE) {
            _Exit(92);
        }

        errno = ERANGE;
        dropped = snapshot();
        if (errno != ERANGE ||
            dropped.ruid != inherited.ruid ||
            dropped.rgid != inherited.rgid ||
            dropped.euid != dropped.ruid ||
            dropped.egid != dropped.rgid) {
            _Exit(93);
        }

        if (dup2(report[1], STDOUT_FILENO) != STDOUT_FILENO) {
            _Exit(94);
        }
        if (close(report[1]) != 0) {
            _Exit(95);
        }

        errno = ERANGE;
        if (write(STDOUT_FILENO, &inherited, sizeof(inherited)) !=
                (ssize_t)sizeof(inherited) ||
            write(STDOUT_FILENO, &dropped, sizeof(dropped)) !=
                (ssize_t)sizeof(dropped) ||
            errno != ERANGE) {
            _Exit(96);
        }

        child_argv[0] = argv[1];
        child_argv[1] = argv[3];
        child_argv[2] = (char *)0;
        empty_envp[0] = (char *)0;
        execve(argv[1], child_argv, empty_envp);
        _Exit(97);
    }

    errno = ERANGE;
    if (close(report[1]) != 0 || errno != ERANGE) {
        close(report[0]);
        unlink(argv[2]);
        return 6;
    }

    errno = ERANGE;
    if (!read_exact(report[0], &inherited, sizeof(inherited)) ||
        !read_exact(report[0], &dropped, sizeof(dropped)) ||
        !read_exact(report[0], &executed, sizeof(executed)) ||
        errno != ERANGE) {
        close(report[0]);
        unlink(argv[2]);
        unlink(argv[3]);
        return 7;
    }

    status = -1;
    errno = ERANGE;
    if (waitpid(child, &status, 0) != child || errno != ERANGE ||
        !WIFEXITED(status) || WEXITSTATUS(status) != 47 ||
        !same_snapshot(&inherited, &parent_before) ||
        dropped.ruid != parent_before.ruid ||
        dropped.rgid != parent_before.rgid ||
        dropped.euid != dropped.ruid ||
        dropped.egid != dropped.rgid ||
        !same_snapshot(&executed, &dropped)) {
        close(report[0]);
        unlink(argv[2]);
        unlink(argv[3]);
        return 8;
    }

    errno = ERANGE;
    if (read(report[0], &status, 1U) != 0 ||
        close(report[0]) != 0 ||
        errno != ERANGE) {
        unlink(argv[2]);
        unlink(argv[3]);
        return 9;
    }

    errno = ERANGE;
    if (stat(argv[3], &child_file) != 0 || errno != ERANGE ||
        child_file.st_uid != executed.euid ||
        child_file.st_gid != executed.egid) {
        unlink(argv[2]);
        unlink(argv[3]);
        return 10;
    }

    errno = ERANGE;
    parent_after = snapshot();
    if (errno != ERANGE ||
        !same_snapshot(&parent_after, &parent_before)) {
        unlink(argv[2]);
        unlink(argv[3]);
        return 11;
    }

    errno = ERANGE;
    if (unlink(argv[2]) != 0 || errno != ERANGE ||
        unlink(argv[3]) != 0 || errno != ERANGE) {
        return 12;
    }

    if (mini_sys_write(STDOUT_FILENO, ok, sizeof(ok) - 1U) !=
        (long)(sizeof(ok) - 1U)) {
        return 13;
    }
    return 0;
}
