#include <errno.h>
#include <fcntl.h>
#include <mini/syscall.h>
#include <poll.h>
#include <stddef.h>
#include <unistd.h>

static int has(short value, short bits)
{
    return (value & bits) == bits;
}

static int has_any(short value, short bits)
{
    return (value & bits) != 0;
}

int main(void)
{
    static const char ok[] = "poll-readiness-ok\n";
    struct pollfd fds[3];
    int pipefd[2];
    int error_pipe[2];
    char buffer[2];
    int result;
    int closed_read_fd;

    errno = ERANGE;
    if (poll((struct pollfd *)0, 0U, 0) != 0 || errno != ERANGE) {
        return 1;
    }

    errno = ERANGE;
    if (pipe2(pipefd, O_NONBLOCK | O_CLOEXEC) != 0 || errno != ERANGE) {
        return 2;
    }

    fds[0].fd = pipefd[0];
    fds[0].events = POLLIN;
    fds[0].revents = (short)-1;
    errno = ERANGE;
    result = poll(fds, 1U, 0);
    if (result != 0 || errno != ERANGE || fds[0].revents != 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        return 3;
    }

    fds[0].fd = pipefd[1];
    fds[0].events = POLLOUT;
    fds[0].revents = 0;
    errno = ERANGE;
    result = poll(fds, 1U, 0);
    if (result != 1 || errno != ERANGE ||
        !has(fds[0].revents, POLLOUT) ||
        has_any(fds[0].revents, POLLERR | POLLHUP | POLLNVAL)) {
        close(pipefd[0]);
        close(pipefd[1]);
        return 4;
    }

    fds[0].fd = pipefd[0];
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    fds[1].fd = pipefd[1];
    fds[1].events = POLLOUT;
    fds[1].revents = 0;
    fds[2].fd = -1;
    fds[2].events = POLLIN | POLLOUT;
    fds[2].revents = (short)-1;
    errno = ERANGE;
    result = poll(fds, 3U, 0);
    if (result != 1 || errno != ERANGE ||
        fds[0].revents != 0 ||
        !has(fds[1].revents, POLLOUT) ||
        fds[2].revents != 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        return 5;
    }

    errno = ERANGE;
    if (write(pipefd[1], "xy", 2U) != 2 || errno != ERANGE) {
        close(pipefd[0]);
        close(pipefd[1]);
        return 6;
    }

    fds[0].fd = pipefd[0];
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    errno = ERANGE;
    result = poll(fds, 1U, 0);
    if (result != 1 || errno != ERANGE ||
        !has(fds[0].revents, POLLIN) ||
        has_any(fds[0].revents, POLLNVAL)) {
        close(pipefd[0]);
        close(pipefd[1]);
        return 7;
    }

    errno = ERANGE;
    if (close(pipefd[1]) != 0 || errno != ERANGE) {
        close(pipefd[0]);
        return 8;
    }

    fds[0].fd = pipefd[0];
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    errno = ERANGE;
    result = poll(fds, 1U, 0);
    if (result != 1 || errno != ERANGE ||
        !has(fds[0].revents, POLLIN | POLLHUP) ||
        has_any(fds[0].revents, POLLNVAL)) {
        close(pipefd[0]);
        return 9;
    }

    errno = ERANGE;
    if (read(pipefd[0], buffer, 2U) != 2 || errno != ERANGE ||
        buffer[0] != 'x' || buffer[1] != 'y') {
        close(pipefd[0]);
        return 10;
    }

    fds[0].fd = pipefd[0];
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    errno = ERANGE;
    result = poll(fds, 1U, 0);
    if (result != 1 || errno != ERANGE ||
        !has(fds[0].revents, POLLHUP) ||
        has_any(fds[0].revents, POLLIN | POLLNVAL)) {
        close(pipefd[0]);
        return 11;
    }

    errno = ERANGE;
    if (read(pipefd[0], buffer, 1U) != 0 || errno != ERANGE) {
        close(pipefd[0]);
        return 12;
    }

    closed_read_fd = pipefd[0];
    if (close(pipefd[0]) != 0) {
        return 13;
    }

    fds[0].fd = closed_read_fd;
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    errno = ERANGE;
    result = poll(fds, 1U, 0);
    if (result != 1 || errno != ERANGE ||
        fds[0].revents != POLLNVAL) {
        return 14;
    }

    fds[0].fd = -1;
    fds[0].events = POLLIN;
    fds[0].revents = (short)-1;
    errno = ERANGE;
    result = poll(fds, 1U, 0);
    if (result != 0 || errno != ERANGE || fds[0].revents != 0) {
        return 15;
    }

    errno = ERANGE;
    if (pipe(error_pipe) != 0 || errno != ERANGE) {
        return 16;
    }
    if (close(error_pipe[0]) != 0) {
        close(error_pipe[1]);
        return 17;
    }

    fds[0].fd = error_pipe[1];
    fds[0].events = POLLOUT;
    fds[0].revents = 0;
    errno = ERANGE;
    result = poll(fds, 1U, 0);
    if (result != 1 || errno != ERANGE ||
        !has(fds[0].revents, POLLERR) ||
        has(fds[0].revents, POLLNVAL)) {
        close(error_pipe[1]);
        return 18;
    }
    if (close(error_pipe[1]) != 0) {
        return 19;
    }

    if (mini_sys_write(STDOUT_FILENO, ok, sizeof(ok) - 1U) !=
        (long)(sizeof(ok) - 1U)) {
        return 20;
    }
    return 0;
}
