#include <errno.h>
#include <fcntl.h>
#include <mini/syscall.h>
#include <poll.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
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
    static const char child_message[] = "kid";
    static const char ok[] = "process-orchestration-ok\n";
    struct pollfd ready;
    char buffer[4];
    int pipefd[2];
    int status;
    int result;
    pid_t child;
    pid_t waited;

    errno = ERANGE;
    if (pipe2(pipefd, O_CLOEXEC) != 0 || errno != ERANGE) {
        return 1;
    }

    errno = ERANGE;
    child = fork();
    if (child < 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        return 2;
    }

    if (child == 0) {
        if (errno != ERANGE) {
            _Exit(90);
        }
        if (close(pipefd[0]) != 0) {
            _Exit(91);
        }
        if (write(pipefd[1], child_message,
                  sizeof(child_message) - 1U) !=
            (ssize_t)(sizeof(child_message) - 1U)) {
            _Exit(92);
        }
        if (close(pipefd[1]) != 0) {
            _Exit(93);
        }
        _Exit(23);
    }

    if (errno != ERANGE || child <= 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        return 3;
    }

    errno = ERANGE;
    if (close(pipefd[1]) != 0 || errno != ERANGE) {
        close(pipefd[0]);
        return 4;
    }

    ready.fd = pipefd[0];
    ready.events = POLLIN;
    ready.revents = 0;
    errno = ERANGE;
    result = poll(&ready, 1U, -1);
    if (result != 1 || errno != ERANGE ||
        (ready.revents & POLLIN) == 0 ||
        (ready.revents & POLLNVAL) != 0) {
        close(pipefd[0]);
        return 5;
    }

    errno = ERANGE;
    if (read(pipefd[0], buffer, sizeof(child_message) - 1U) !=
            (ssize_t)(sizeof(child_message) - 1U) ||
        errno != ERANGE ||
        !same_bytes(buffer, child_message, sizeof(child_message) - 1U)) {
        close(pipefd[0]);
        return 6;
    }

    status = -1;
    errno = ERANGE;
    waited = waitpid(child, &status, 0);
    if (waited != child || errno != ERANGE ||
        !WIFEXITED(status) || WEXITSTATUS(status) != 23) {
        close(pipefd[0]);
        return 7;
    }

    errno = ERANGE;
    if (read(pipefd[0], buffer, 1U) != 0 || errno != ERANGE) {
        close(pipefd[0]);
        return 8;
    }

    ready.fd = pipefd[0];
    ready.events = POLLIN;
    ready.revents = 0;
    errno = ERANGE;
    result = poll(&ready, 1U, 0);
    if (result != 1 || errno != ERANGE ||
        (ready.revents & POLLHUP) == 0 ||
        (ready.revents & POLLNVAL) != 0) {
        close(pipefd[0]);
        return 9;
    }

    errno = ERANGE;
    if (close(pipefd[0]) != 0 || errno != ERANGE) {
        return 10;
    }

    status = -1;
    errno = ERANGE;
    if (waitpid(child, &status, 0) != (pid_t)-1 || errno != ECHILD) {
        return 11;
    }

    errno = ERANGE;
    if (waitpid((pid_t)-1, &status, WNOHANG) != (pid_t)-1 ||
        errno != ECHILD) {
        return 12;
    }

    if (mini_sys_write(STDOUT_FILENO, ok, sizeof(ok) - 1U) !=
        (long)(sizeof(ok) - 1U)) {
        return 13;
    }
    return 0;
}
