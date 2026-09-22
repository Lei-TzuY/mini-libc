#include <errno.h>
#include <fcntl.h>
#include <mini/syscall.h>
#include <signal.h>
#include <stddef.h>
#include <string.h>
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

int main(void)
{
    static const char ok[] = "pipe-ipc-ok\n";
    int plain[2];
    int nonblock[2];
    int broken[2];
    int duplicate;
    int read_flags;
    int write_flags;
    char buffer[8];
    void (*previous)(int);

    broken[0] = -1;
    broken[1] = -1;

    errno = ERANGE;
    if (pipe(plain) != 0 || errno != ERANGE) {
        return 1;
    }

    errno = ERANGE;
    read_flags = fcntl(plain[0], F_GETFL);
    write_flags = fcntl(plain[1], F_GETFL);
    if (read_flags < 0 || write_flags < 0 || errno != ERANGE ||
        (read_flags & O_ACCMODE) != O_RDONLY ||
        (write_flags & O_ACCMODE) != O_WRONLY ||
        (read_flags & O_NONBLOCK) != 0 ||
        (write_flags & O_NONBLOCK) != 0 ||
        fcntl(plain[0], F_GETFD) != 0 ||
        fcntl(plain[1], F_GETFD) != 0) {
        close(plain[0]);
        close(plain[1]);
        return 2;
    }

    errno = ERANGE;
    if (write(plain[1], "abc", 3U) != 3 || errno != ERANGE ||
        read(plain[0], buffer, 3U) != 3 || errno != ERANGE ||
        !same_bytes(buffer, "abc", 3U)) {
        close(plain[0]);
        close(plain[1]);
        return 3;
    }

    errno = ERANGE;
    if (close(plain[1]) != 0 || errno != ERANGE ||
        read(plain[0], buffer, 1U) != 0 || errno != ERANGE ||
        close(plain[0]) != 0 || errno != ERANGE) {
        return 4;
    }

    errno = ERANGE;
    if (pipe2(nonblock, O_NONBLOCK | O_CLOEXEC) != 0 || errno != ERANGE) {
        return 5;
    }

    errno = ERANGE;
    read_flags = fcntl(nonblock[0], F_GETFL);
    write_flags = fcntl(nonblock[1], F_GETFL);
    if (read_flags < 0 || write_flags < 0 || errno != ERANGE ||
        (read_flags & O_ACCMODE) != O_RDONLY ||
        (write_flags & O_ACCMODE) != O_WRONLY ||
        (read_flags & O_NONBLOCK) == 0 ||
        (write_flags & O_NONBLOCK) == 0 ||
        fcntl(nonblock[0], F_GETFD) != FD_CLOEXEC ||
        fcntl(nonblock[1], F_GETFD) != FD_CLOEXEC) {
        close(nonblock[0]);
        close(nonblock[1]);
        return 6;
    }

    errno = ERANGE;
    if (read(nonblock[0], buffer, 1U) != (ssize_t)-1 ||
        errno != EAGAIN) {
        close(nonblock[0]);
        close(nonblock[1]);
        return 7;
    }

    errno = ERANGE;
    if (write(nonblock[1], "xy", 2U) != 2 || errno != ERANGE ||
        read(nonblock[0], buffer, 2U) != 2 || errno != ERANGE ||
        !same_bytes(buffer, "xy", 2U)) {
        close(nonblock[0]);
        close(nonblock[1]);
        return 8;
    }

    errno = ERANGE;
    duplicate = dup(nonblock[1]);
    if (duplicate < 0 || errno != ERANGE ||
        fcntl(duplicate, F_GETFD) != 0 || errno != ERANGE ||
        (fcntl(duplicate, F_GETFL) & O_NONBLOCK) == 0 ||
        errno != ERANGE) {
        if (duplicate >= 0) {
            close(duplicate);
        }
        close(nonblock[0]);
        close(nonblock[1]);
        return 9;
    }

    errno = ERANGE;
    if (close(nonblock[1]) != 0 || errno != ERANGE) {
        close(duplicate);
        close(nonblock[0]);
        return 10;
    }

    errno = ERANGE;
    if (read(nonblock[0], buffer, 1U) != (ssize_t)-1 ||
        errno != EAGAIN) {
        close(duplicate);
        close(nonblock[0]);
        return 11;
    }

    errno = ERANGE;
    if (close(duplicate) != 0 || errno != ERANGE ||
        read(nonblock[0], buffer, 1U) != 0 || errno != ERANGE ||
        close(nonblock[0]) != 0 || errno != ERANGE) {
        return 12;
    }

    errno = ERANGE;
    previous = signal(SIGPIPE, SIG_IGN);
    if (previous == SIG_ERR || errno != ERANGE) {
        return 13;
    }

    errno = ERANGE;
    if (pipe(broken) != 0 || errno != ERANGE ||
        close(broken[0]) != 0 || errno != ERANGE) {
        (void)signal(SIGPIPE, previous);
        if (broken[1] >= 0) {
            (void)close(broken[1]);
        }
        return 14;
    }

    errno = ERANGE;
    if (write(broken[1], "z", 1U) != (ssize_t)-1 || errno != EPIPE) {
        (void)close(broken[1]);
        (void)signal(SIGPIPE, previous);
        return 15;
    }
    if (close(broken[1]) != 0) {
        (void)signal(SIGPIPE, previous);
        return 16;
    }

    errno = ERANGE;
    if (signal(SIGPIPE, previous) == SIG_ERR || errno != ERANGE) {
        return 17;
    }

    plain[0] = -1;
    plain[1] = -1;
    errno = ERANGE;
    if (pipe2(plain, O_APPEND) != -1 || errno != EINVAL) {
        if (plain[0] >= 0) {
            close(plain[0]);
        }
        if (plain[1] >= 0) {
            close(plain[1]);
        }
        return 18;
    }

    if (mini_sys_write(STDOUT_FILENO, ok, sizeof(ok) - 1U) !=
        (long)(sizeof(ok) - 1U)) {
        return 19;
    }
    return 0;
}
