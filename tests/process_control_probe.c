#include <errno.h>
#include <mini/syscall.h>
#include <signal.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

struct process_identity {
    pid_t self;
    pid_t parent;
};

int main(void)
{
    static const char ok[] = "process-control-ok\n";
    struct process_identity identity;
    char control_byte;
    int identity_pipe[2];
    int control_pipe[2];
    int status;
    pid_t parent_pid;
    pid_t parent_parent_pid;
    pid_t child;
    pid_t waited;

    errno = ERANGE;
    parent_pid = getpid();
    parent_parent_pid = getppid();
    if (parent_pid <= 0 || parent_parent_pid <= 0 || errno != ERANGE) {
        return 1;
    }

    errno = ERANGE;
    if (pipe(identity_pipe) != 0 || errno != ERANGE ||
        pipe(control_pipe) != 0 || errno != ERANGE) {
        return 2;
    }

    errno = ERANGE;
    child = fork();
    if (child < 0) {
        close(identity_pipe[0]);
        close(identity_pipe[1]);
        close(control_pipe[0]);
        close(control_pipe[1]);
        return 3;
    }

    if (child == 0) {
        struct process_identity child_identity;

        if (errno != ERANGE ||
            close(identity_pipe[0]) != 0 ||
            close(control_pipe[1]) != 0) {
            _Exit(90);
        }

        errno = ERANGE;
        child_identity.self = getpid();
        child_identity.parent = getppid();
        if (child_identity.self <= 0 || child_identity.parent <= 0 ||
            errno != ERANGE) {
            _Exit(91);
        }

        errno = ERANGE;
        if (write(identity_pipe[1], &child_identity,
                  sizeof(child_identity)) !=
                (ssize_t)sizeof(child_identity) ||
            errno != ERANGE) {
            _Exit(92);
        }
        if (close(identity_pipe[1]) != 0) {
            _Exit(93);
        }

        /*
         * Keep the child deterministically alive until the parent signals it.
         * The parent retains the write end without sending data, so this read
         * blocks instead of relying on scheduling delays.
         */
        if (read(control_pipe[0], &control_byte, 1U) >= 0) {
            _Exit(94);
        }
        _Exit(95);
    }

    if (errno != ERANGE || child <= 0 || getpid() != parent_pid ||
        errno != ERANGE) {
        close(identity_pipe[0]);
        close(identity_pipe[1]);
        close(control_pipe[0]);
        close(control_pipe[1]);
        return 4;
    }

    errno = ERANGE;
    if (close(identity_pipe[1]) != 0 ||
        close(control_pipe[0]) != 0 ||
        errno != ERANGE) {
        close(identity_pipe[0]);
        close(control_pipe[1]);
        return 5;
    }

    errno = ERANGE;
    if (read(identity_pipe[0], &identity, sizeof(identity)) !=
            (ssize_t)sizeof(identity) ||
        errno != ERANGE ||
        identity.self != child ||
        identity.parent != parent_pid) {
        close(identity_pipe[0]);
        close(control_pipe[1]);
        return 6;
    }

    errno = ERANGE;
    if (read(identity_pipe[0], &control_byte, 1U) != 0 ||
        close(identity_pipe[0]) != 0 ||
        errno != ERANGE) {
        close(control_pipe[1]);
        return 7;
    }

    errno = ERANGE;
    if (kill(child, 0) != 0 || errno != ERANGE) {
        close(control_pipe[1]);
        return 8;
    }

    errno = ERANGE;
    if (kill(child, SIGTERM) != 0 || errno != ERANGE) {
        close(control_pipe[1]);
        return 9;
    }

    status = -1;
    errno = ERANGE;
    waited = waitpid(child, &status, 0);
    if (waited != child || errno != ERANGE ||
        WIFEXITED(status) ||
        !WIFSIGNALED(status) ||
        WTERMSIG(status) != SIGTERM) {
        close(control_pipe[1]);
        return 10;
    }

    errno = ERANGE;
    if (close(control_pipe[1]) != 0 || errno != ERANGE) {
        return 11;
    }

    errno = ERANGE;
    if (kill((pid_t)2147483647, 0) != -1 || errno != ESRCH) {
        return 12;
    }

    if (mini_sys_write(STDOUT_FILENO, ok, sizeof(ok) - 1U) !=
        (long)(sizeof(ok) - 1U)) {
        return 13;
    }
    return 0;
}
