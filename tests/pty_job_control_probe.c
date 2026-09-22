#include <errno.h>
#include <fcntl.h>
#include <mini/syscall.h>
#include <mini/tty_ioctl.h>
#include <poll.h>
#include <signal.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static const char marker[] = "pty-job-ok";

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

static _Noreturn void session_child(int master)
{
    struct pollfd unused;
    char command;
    int command_pipe[2];
    int tty_sid = -1;
    int status = -1;
    int slave;
    pid_t self;
    pid_t worker;
    pid_t waited;

    (void)unused;

    errno = ERANGE;
    self = getpid();
    if (self <= 0 || errno != ERANGE ||
        setsid() != self || errno != ERANGE ||
        getpgrp() != self || errno != ERANGE ||
        getsid((pid_t)0) != self || errno != ERANGE) {
        _Exit(90);
    }

    errno = ERANGE;
    slave = (int)mini_sys_ioctl(master, MINI_TIOCGPTPEER,
                                (unsigned long)(O_RDWR | O_CLOEXEC |
                                                O_NOCTTY));
    if (slave < 0) {
        _Exit(91);
    }
    if (close(master) != 0) {
        _Exit(92);
    }

    errno = ERANGE;
    if (mini_sys_ioctl(slave, MINI_TIOCSCTTY, 0UL) < 0L ||
        mini_sys_ioctl(slave, MINI_TIOCGSID,
                       (unsigned long)&tty_sid) < 0L ||
        tty_sid != self ||
        errno != ERANGE) {
        _Exit(93);
    }

    errno = ERANGE;
    if (tcsetpgrp(slave, self) != 0 || errno != ERANGE ||
        tcgetpgrp(slave) != self || errno != ERANGE) {
        _Exit(94);
    }

    errno = ERANGE;
    if (pipe(command_pipe) != 0 || errno != ERANGE) {
        _Exit(95);
    }

    errno = ERANGE;
    worker = fork();
    if (worker < 0) {
        _Exit(96);
    }
    if (worker == 0) {
        pid_t worker_self;

        if (close(command_pipe[1]) != 0) {
            _Exit(100);
        }

        if (read(command_pipe[0], &command, 1U) != 1) {
            _Exit(101);
        }

        errno = ERANGE;
        worker_self = getpid();
        if (worker_self <= 0 ||
            getpgrp() != worker_self ||
            getsid((pid_t)0) != self ||
            errno != ERANGE) {
            _Exit(102);
        }

        errno = ERANGE;
        if (write(slave, marker, sizeof(marker) - 1U) !=
                (ssize_t)(sizeof(marker) - 1U) ||
            errno != ERANGE) {
            _Exit(103);
        }

        if (close(command_pipe[0]) != 0 ||
            close(slave) != 0) {
            _Exit(104);
        }
        _Exit(49);
    }

    if (errno != ERANGE || worker <= 0 ||
        close(command_pipe[0]) != 0) {
        _Exit(97);
    }

    errno = ERANGE;
    if (setpgid(worker, worker) != 0 || errno != ERANGE ||
        getpgid(worker) != worker || errno != ERANGE ||
        getsid(worker) != self || errno != ERANGE ||
        tcsetpgrp(slave, worker) != 0 || errno != ERANGE ||
        tcgetpgrp(slave) != worker || errno != ERANGE) {
        (void)kill(worker, SIGTERM);
        (void)waitpid(worker, &status, 0);
        _Exit(98);
    }

    command = 'G';
    errno = ERANGE;
    if (write(command_pipe[1], &command, 1U) != 1 ||
        close(command_pipe[1]) != 0 ||
        errno != ERANGE) {
        (void)kill(worker, SIGTERM);
        (void)waitpid(worker, &status, 0);
        _Exit(99);
    }

    errno = ERANGE;
    waited = waitpid(worker, &status, 0);
    if (waited != worker || errno != ERANGE ||
        !WIFEXITED(status) || WEXITSTATUS(status) != 49) {
        _Exit(105);
    }

    if (close(slave) != 0) {
        _Exit(106);
    }
    _Exit(48);
}

int main(void)
{
    static const char ok[] = "pty-job-control-ok\n";
    struct pollfd ready;
    char buffer[sizeof(marker)];
    int pipefd[2];
    int unlock = 0;
    int master;
    int status = -1;
    int count;
    pid_t child;

    errno = ERANGE;
    if (pipe(pipefd) != 0 || errno != ERANGE) {
        return 1;
    }
    errno = ERANGE;
    if (tcgetpgrp(pipefd[0]) != (pid_t)-1 || errno != ENOTTY) {
        close(pipefd[0]);
        close(pipefd[1]);
        return 2;
    }
    if (close(pipefd[0]) != 0 || close(pipefd[1]) != 0) {
        return 3;
    }

    errno = ERANGE;
    master = open("/dev/ptmx", O_RDWR | O_CLOEXEC | O_NOCTTY);
    if (master < 0 || errno != ERANGE) {
        return 4;
    }

    errno = ERANGE;
    if (mini_sys_ioctl(master, MINI_TIOCSPTLCK,
                       (unsigned long)&unlock) < 0L ||
        errno != ERANGE) {
        close(master);
        return 5;
    }

    errno = ERANGE;
    child = fork();
    if (child < 0) {
        close(master);
        return 6;
    }
    if (child == 0) {
        session_child(master);
    }
    if (errno != ERANGE || child <= 0) {
        close(master);
        return 7;
    }

    ready.fd = master;
    ready.events = POLLIN;
    ready.revents = 0;
    errno = ERANGE;
    if (poll(&ready, 1U, -1) != 1 ||
        errno != ERANGE ||
        (ready.revents & POLLIN) == 0 ||
        (ready.revents & POLLNVAL) != 0) {
        kill(child, SIGTERM);
        waitpid(child, &status, 0);
        close(master);
        return 8;
    }

    errno = ERANGE;
    count = (int)read(master, buffer, sizeof(marker) - 1U);
    if (count != (int)(sizeof(marker) - 1U) ||
        errno != ERANGE ||
        !same_bytes(buffer, marker, sizeof(marker) - 1U)) {
        kill(child, SIGTERM);
        waitpid(child, &status, 0);
        close(master);
        return 9;
    }

    errno = ERANGE;
    if (waitpid(child, &status, 0) != child ||
        errno != ERANGE ||
        !WIFEXITED(status) ||
        WEXITSTATUS(status) != 48) {
        close(master);
        return 10;
    }

    errno = ERANGE;
    if (close(master) != 0 || errno != ERANGE) {
        return 11;
    }

    if (mini_sys_write(STDOUT_FILENO, ok, sizeof(ok) - 1U) !=
        (long)(sizeof(ok) - 1U)) {
        return 12;
    }
    return 0;
}
