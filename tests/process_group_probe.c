#include <errno.h>
#include <mini/syscall.h>
#include <signal.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

struct child_ready {
    pid_t self;
    pid_t parent;
};

static _Noreturn void child_main(int ready_write, int control_read)
{
    struct child_ready ready;
    char byte;

    errno = ERANGE;
    ready.self = getpid();
    ready.parent = getppid();
    if (ready.self <= 0 || ready.parent <= 0 || errno != ERANGE) {
        _Exit(90);
    }

    errno = ERANGE;
    if (write(ready_write, &ready, sizeof(ready)) != (ssize_t)sizeof(ready) ||
        errno != ERANGE) {
        _Exit(91);
    }
    if (close(ready_write) != 0) {
        _Exit(92);
    }

    /*
     * The parent keeps the write end open without sending data. This gives us
     * a deterministic alive-and-blocked child while group membership changes.
     */
    if (read(control_read, &byte, 1U) >= 0) {
        _Exit(93);
    }
    _Exit(94);
}

static int record_matches(const struct child_ready *record,
                          pid_t child1, pid_t child2, pid_t parent)
{
    return record->parent == parent &&
           (record->self == child1 || record->self == child2);
}

int main(void)
{
    static const char ok[] = "process-group-ok\n";
    struct child_ready records[2];
    int ready_pipe[2];
    int control_pipe[2];
    int status;
    int saw_child1 = 0;
    int saw_child2 = 0;
    pid_t parent;
    pid_t original_group;
    pid_t group;
    pid_t child1;
    pid_t child2;
    pid_t waited1;
    pid_t waited2;

    ready_pipe[0] = -1;
    ready_pipe[1] = -1;
    control_pipe[0] = -1;
    control_pipe[1] = -1;

    errno = ERANGE;
    parent = getpid();
    original_group = getpgrp();
    if (parent <= 0 || original_group <= 0 || errno != ERANGE ||
        getpgid((pid_t)0) != original_group || errno != ERANGE) {
        return 1;
    }

    errno = ERANGE;
    if (pipe(ready_pipe) != 0 || errno != ERANGE ||
        pipe(control_pipe) != 0 || errno != ERANGE) {
        return 2;
    }

    errno = ERANGE;
    child1 = fork();
    if (child1 < 0) {
        return 3;
    }
    if (child1 == 0) {
        (void)close(ready_pipe[0]);
        (void)close(control_pipe[1]);
        child_main(ready_pipe[1], control_pipe[0]);
    }
    if (errno != ERANGE || child1 <= 0) {
        return 4;
    }

    errno = ERANGE;
    if (setpgid(child1, child1) != 0 || errno != ERANGE ||
        getpgid(child1) != child1 || errno != ERANGE) {
        (void)kill(child1, SIGTERM);
        (void)waitpid(child1, &status, 0);
        return 5;
    }
    group = child1;

    errno = ERANGE;
    child2 = fork();
    if (child2 < 0) {
        (void)kill(child1, SIGTERM);
        (void)waitpid(child1, &status, 0);
        return 6;
    }
    if (child2 == 0) {
        (void)close(ready_pipe[0]);
        (void)close(control_pipe[1]);
        child_main(ready_pipe[1], control_pipe[0]);
    }
    if (errno != ERANGE || child2 <= 0) {
        return 7;
    }

    errno = ERANGE;
    if (setpgid(child2, group) != 0 || errno != ERANGE ||
        getpgid(child2) != group || errno != ERANGE ||
        getpgid(child1) != group || errno != ERANGE ||
        getpgrp() != original_group || errno != ERANGE) {
        (void)kill(-group, SIGTERM);
        (void)waitpid(child1, &status, 0);
        (void)waitpid(child2, &status, 0);
        return 8;
    }

    errno = ERANGE;
    if (close(ready_pipe[1]) != 0 ||
        close(control_pipe[0]) != 0 ||
        errno != ERANGE) {
        (void)kill(-group, SIGTERM);
        return 9;
    }

    errno = ERANGE;
    if (read(ready_pipe[0], &records[0], sizeof(records[0])) !=
            (ssize_t)sizeof(records[0]) ||
        read(ready_pipe[0], &records[1], sizeof(records[1])) !=
            (ssize_t)sizeof(records[1]) ||
        errno != ERANGE ||
        !record_matches(&records[0], child1, child2, parent) ||
        !record_matches(&records[1], child1, child2, parent) ||
        records[0].self == records[1].self) {
        (void)kill(-group, SIGTERM);
        return 10;
    }
    saw_child1 = records[0].self == child1 || records[1].self == child1;
    saw_child2 = records[0].self == child2 || records[1].self == child2;
    if (!saw_child1 || !saw_child2) {
        (void)kill(-group, SIGTERM);
        return 11;
    }

    errno = ERANGE;
    if (read(ready_pipe[0], &records[0], 1U) != 0 ||
        close(ready_pipe[0]) != 0 ||
        errno != ERANGE) {
        (void)kill(-group, SIGTERM);
        return 12;
    }

    errno = ERANGE;
    if (kill(-group, 0) != 0 || errno != ERANGE ||
        kill(-group, SIGTERM) != 0 || errno != ERANGE) {
        return 13;
    }

    status = -1;
    errno = ERANGE;
    waited1 = waitpid(-group, &status, 0);
    if ((waited1 != child1 && waited1 != child2) ||
        errno != ERANGE || !WIFSIGNALED(status) ||
        WTERMSIG(status) != SIGTERM) {
        return 14;
    }

    status = -1;
    errno = ERANGE;
    waited2 = waitpid(-group, &status, 0);
    if ((waited2 != child1 && waited2 != child2) ||
        waited2 == waited1 ||
        errno != ERANGE || !WIFSIGNALED(status) ||
        WTERMSIG(status) != SIGTERM) {
        return 15;
    }

    status = -1;
    errno = ERANGE;
    if (waitpid(-group, &status, WNOHANG) != (pid_t)-1 ||
        errno != ECHILD) {
        return 16;
    }

    errno = ERANGE;
    if (getpgrp() != original_group || errno != ERANGE ||
        getpgid((pid_t)0) != original_group || errno != ERANGE) {
        return 17;
    }

    errno = ERANGE;
    if (close(control_pipe[1]) != 0 || errno != ERANGE) {
        return 18;
    }

    if (mini_sys_write(STDOUT_FILENO, ok, sizeof(ok) - 1U) !=
        (long)(sizeof(ok) - 1U)) {
        return 19;
    }
    return 0;
}
