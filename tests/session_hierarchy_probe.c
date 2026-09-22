#include <errno.h>
#include <mini/syscall.h>
#include <signal.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

struct session_report {
    pid_t self;
    pid_t parent;
    pid_t sid_before;
    pid_t pgrp_before;
    pid_t setsid_result;
    pid_t sid_after;
    pid_t pgrp_after;
    int second_setsid_errno;
    int setpgid_errno;
};

int main(void)
{
    static const char ok[] = "session-hierarchy-ok\n";
    struct session_report report;
    char byte;
    int report_pipe[2];
    int control_pipe[2];
    int status;
    pid_t parent_pid;
    pid_t parent_sid;
    pid_t parent_pgrp;
    pid_t child;
    pid_t waited;

    report_pipe[0] = -1;
    report_pipe[1] = -1;
    control_pipe[0] = -1;
    control_pipe[1] = -1;

    errno = ERANGE;
    parent_pid = getpid();
    parent_sid = getsid((pid_t)0);
    parent_pgrp = getpgrp();
    if (parent_pid <= 0 || parent_sid <= 0 || parent_pgrp <= 0 ||
        errno != ERANGE ||
        getsid(parent_pid) != parent_sid || errno != ERANGE) {
        return 1;
    }

    errno = ERANGE;
    if (pipe(report_pipe) != 0 || errno != ERANGE ||
        pipe(control_pipe) != 0 || errno != ERANGE) {
        return 2;
    }

    errno = ERANGE;
    child = fork();
    if (child < 0) {
        return 3;
    }

    if (child == 0) {
        if (close(report_pipe[0]) != 0 ||
            close(control_pipe[1]) != 0) {
            _Exit(90);
        }

        errno = ERANGE;
        report.self = getpid();
        report.parent = getppid();
        report.sid_before = getsid((pid_t)0);
        report.pgrp_before = getpgrp();
        if (report.self <= 0 || report.parent <= 0 ||
            report.sid_before <= 0 || report.pgrp_before <= 0 ||
            errno != ERANGE) {
            _Exit(91);
        }

        errno = ERANGE;
        report.setsid_result = setsid();
        if (report.setsid_result != report.self || errno != ERANGE) {
            _Exit(92);
        }

        errno = ERANGE;
        report.sid_after = getsid((pid_t)0);
        report.pgrp_after = getpgrp();
        if (report.sid_after != report.self ||
            report.pgrp_after != report.self ||
            getpgid((pid_t)0) != report.self ||
            errno != ERANGE) {
            _Exit(93);
        }

        errno = ERANGE;
        if (setsid() != (pid_t)-1 || errno != EPERM) {
            _Exit(94);
        }
        report.second_setsid_errno = errno;

        errno = ERANGE;
        if (setpgid((pid_t)0, (pid_t)0) != -1 || errno != EPERM) {
            _Exit(95);
        }
        report.setpgid_errno = errno;

        errno = ERANGE;
        if (write(report_pipe[1], &report, sizeof(report)) !=
                (ssize_t)sizeof(report) ||
            errno != ERANGE ||
            close(report_pipe[1]) != 0) {
            _Exit(96);
        }

        /*
         * Parent keeps the writer open without sending data. The child stays
         * alive in its new session until the parent has verified it.
         */
        if (read(control_pipe[0], &byte, 1U) >= 0) {
            _Exit(97);
        }
        _Exit(98);
    }

    if (errno != ERANGE || child <= 0) {
        return 4;
    }

    errno = ERANGE;
    if (close(report_pipe[1]) != 0 ||
        close(control_pipe[0]) != 0 ||
        errno != ERANGE) {
        return 5;
    }

    errno = ERANGE;
    if (read(report_pipe[0], &report, sizeof(report)) !=
            (ssize_t)sizeof(report) ||
        errno != ERANGE ||
        report.self != child ||
        report.parent != parent_pid ||
        report.sid_before != parent_sid ||
        report.pgrp_before != parent_pgrp ||
        report.setsid_result != child ||
        report.sid_after != child ||
        report.pgrp_after != child ||
        report.second_setsid_errno != EPERM ||
        report.setpgid_errno != EPERM) {
        (void)kill(child, SIGTERM);
        return 6;
    }

    errno = ERANGE;
    if (read(report_pipe[0], &byte, 1U) != 0 ||
        close(report_pipe[0]) != 0 ||
        errno != ERANGE) {
        (void)kill(child, SIGTERM);
        return 7;
    }

    errno = ERANGE;
    if (getsid(child) != child || errno != ERANGE ||
        getpgid(child) != child || errno != ERANGE ||
        getsid((pid_t)0) != parent_sid || errno != ERANGE ||
        getpgrp() != parent_pgrp || errno != ERANGE) {
        (void)kill(child, SIGTERM);
        return 8;
    }

    errno = ERANGE;
    if (getsid((pid_t)2147483647) != (pid_t)-1 || errno != ESRCH) {
        (void)kill(child, SIGTERM);
        return 9;
    }

    errno = ERANGE;
    if (kill(child, SIGTERM) != 0 || errno != ERANGE) {
        return 10;
    }

    status = -1;
    errno = ERANGE;
    waited = waitpid(child, &status, 0);
    if (waited != child || errno != ERANGE ||
        !WIFSIGNALED(status) || WTERMSIG(status) != SIGTERM) {
        return 11;
    }

    errno = ERANGE;
    if (close(control_pipe[1]) != 0 || errno != ERANGE ||
        getsid((pid_t)0) != parent_sid || errno != ERANGE ||
        getpgrp() != parent_pgrp || errno != ERANGE) {
        return 12;
    }

    if (mini_sys_write(STDOUT_FILENO, ok, sizeof(ok) - 1U) !=
        (long)(sizeof(ok) - 1U)) {
        return 13;
    }
    return 0;
}
