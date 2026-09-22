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

int main(int argc, char **argv)
{
    static const char expected[] = "exec-child-ok\n";
    static char token[] = "MINI_EXEC_TOKEN=launch-ok";
    static char arg0[] = "exec-child";
    static char arg1[] = "alpha";
    static char arg2[] = "beta";
    static char missing[] = "mini-libc-exec-definitely-missing";
    char *child_argv[4];
    char *child_envp[2];
    char *missing_argv[2];
    struct pollfd ready;
    char buffer[32];
    int pipefd[2];
    int status;
    int count;
    pid_t child;
    pid_t waited;

    if (argc != 2) {
        return 1;
    }

    missing_argv[0] = missing;
    missing_argv[1] = (char *)0;
    errno = ERANGE;
    if (execve(missing, missing_argv, (char *const *)0) != -1 ||
        errno != ENOENT) {
        return 2;
    }

    errno = ERANGE;
    if (pipe2(pipefd, O_CLOEXEC) != 0 || errno != ERANGE) {
        return 3;
    }

    errno = ERANGE;
    child = fork();
    if (child < 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        return 4;
    }

    if (child == 0) {
        if (close(pipefd[0]) != 0) {
            _Exit(90);
        }
        if (dup2(pipefd[1], STDOUT_FILENO) != STDOUT_FILENO) {
            _Exit(91);
        }
        if (dup2(pipefd[1], 100) != 100) {
            _Exit(92);
        }
        if (fcntl(100, F_SETFD, FD_CLOEXEC) != 0) {
            _Exit(93);
        }
        if (close(pipefd[1]) != 0) {
            _Exit(94);
        }

        child_argv[0] = arg0;
        child_argv[1] = arg1;
        child_argv[2] = arg2;
        child_argv[3] = (char *)0;
        child_envp[0] = token;
        child_envp[1] = (char *)0;

        execve(argv[1], child_argv, child_envp);
        _Exit(95);
    }

    if (errno != ERANGE || child <= 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        return 5;
    }

    errno = ERANGE;
    if (close(pipefd[1]) != 0 || errno != ERANGE) {
        close(pipefd[0]);
        return 6;
    }

    ready.fd = pipefd[0];
    ready.events = POLLIN;
    ready.revents = 0;
    errno = ERANGE;
    if (poll(&ready, 1U, -1) != 1 || errno != ERANGE ||
        (ready.revents & POLLIN) == 0 ||
        (ready.revents & POLLNVAL) != 0) {
        close(pipefd[0]);
        return 7;
    }

    errno = ERANGE;
    count = (int)read(pipefd[0], buffer, sizeof(buffer));
    if (count != (int)(sizeof(expected) - 1U) ||
        errno != ERANGE ||
        !same_bytes(buffer, expected, sizeof(expected) - 1U)) {
        close(pipefd[0]);
        return 8;
    }

    status = -1;
    errno = ERANGE;
    waited = waitpid(child, &status, 0);
    if (waited != child || errno != ERANGE ||
        !WIFEXITED(status) || WEXITSTATUS(status) != 37) {
        close(pipefd[0]);
        return 9;
    }

    errno = ERANGE;
    if (read(pipefd[0], buffer, 1U) != 0 || errno != ERANGE ||
        close(pipefd[0]) != 0 || errno != ERANGE) {
        return 10;
    }

    if (mini_sys_write(STDOUT_FILENO, "exec-transition-ok\n", 19U) != 19L) {
        return 11;
    }
    return 0;
}
