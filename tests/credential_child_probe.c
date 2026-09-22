#include <errno.h>
#include <fcntl.h>
#include <mini/syscall.h>
#include <stddef.h>
#include <sys/stat.h>
#include <sys/types.h>
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

int main(int argc, char **argv)
{
    struct credential_snapshot current;
    struct stat info;
    int fd;

    if (argc != 2) {
        return 41;
    }

    errno = ERANGE;
    current = snapshot();
    if (errno != ERANGE ||
        current.euid != current.ruid ||
        current.egid != current.rgid) {
        return 42;
    }

    errno = ERANGE;
    fd = open(argv[1], O_CREAT | O_EXCL | O_RDWR, 0600U);
    if (fd < 0 || errno != ERANGE ||
        fstat(fd, &info) != 0 || errno != ERANGE ||
        info.st_uid != current.euid ||
        info.st_gid != current.egid ||
        close(fd) != 0 || errno != ERANGE) {
        if (fd >= 0) {
            (void)close(fd);
        }
        return 43;
    }

    errno = ERANGE;
    if (write(STDOUT_FILENO, &current, sizeof(current)) !=
            (ssize_t)sizeof(current) ||
        errno != ERANGE) {
        return 44;
    }

    return 47;
}
