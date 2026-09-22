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
#include <termios.h>
#include <unistd.h>

static const char resume_marker[] = "tty-resume-ok";

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

static int write_phase(int fd, char phase)
{
    return write(fd, &phase, 1U) == 1;
}

static _Noreturn void session_child(int master, int phase_write)
{
    struct termios original;
    struct termios signaling;
    char command;
    int hold[2];
    int slave;
    int status;
    pid_t self;
    pid_t worker;
    pid_t waited;

    errno = ERANGE;
    self = getpid();
    if (self <= 0 || errno != ERANGE ||
        setsid() != self || errno != ERANGE ||
        getpgrp() != self || errno != ERANGE ||
        getsid((pid_t)0) != self || errno != ERANGE) {
        _Exit(90);
    }

    if (signal(SIGTTOU, SIG_IGN) == SIG_ERR) {
        _Exit(91);
    }

    errno = ERANGE;
    slave = (int)mini_sys_ioctl(master, MINI_TIOCGPTPEER,
                                (unsigned long)(O_RDWR | O_CLOEXEC |
                                                O_NOCTTY));
    if (slave < 0 || errno != ERANGE) {
        _Exit(92);
    }
    if (close(master) != 0) {
        _Exit(93);
    }

    errno = ERANGE;
    if (mini_sys_ioctl(slave, MINI_TIOCSCTTY, 0UL) < 0L ||
        errno != ERANGE ||
        tcsetpgrp(slave, self) != 0 || errno != ERANGE ||
        tcgetpgrp(slave) != self || errno != ERANGE ||
        tcgetattr(slave, &original) != 0 || errno != ERANGE) {
        _Exit(94);
    }

    signaling = original;
    signaling.c_lflag |= (ISIG | ICANON);
    signaling.c_lflag &= ~ECHO;
    signaling.c_cc[VINTR] = 3U;
    signaling.c_cc[VSUSP] = 26U;

    errno = ERANGE;
    if (tcsetattr(slave, TCSANOW, &signaling) != 0 || errno != ERANGE) {
        _Exit(95);
    }

    errno = ERANGE;
    if (pipe(hold) != 0 || errno != ERANGE) {
        _Exit(96);
    }

    errno = ERANGE;
    worker = fork();
    if (worker < 0) {
        _Exit(97);
    }
    if (worker == 0) {
        if (close(hold[1]) != 0) {
            _Exit(110);
        }
        if (read(hold[0], &command, 1U) >= 0) {
            _Exit(111);
        }
        _Exit(112);
    }

    if (errno != ERANGE || worker <= 0 ||
        close(hold[0]) != 0) {
        _Exit(98);
    }

    errno = ERANGE;
    if (setpgid(worker, worker) != 0 || errno != ERANGE ||
        tcsetpgrp(slave, worker) != 0 || errno != ERANGE ||
        tcgetpgrp(slave) != worker || errno != ERANGE ||
        !write_phase(phase_write, 'I')) {
        (void)kill(worker, SIGTERM);
        (void)waitpid(worker, &status, 0);
        _Exit(99);
    }

    status = -1;
    errno = ERANGE;
    waited = waitpid(worker, &status, 0);
    if (waited != worker || errno != ERANGE ||
        !WIFSIGNALED(status) || WTERMSIG(status) != SIGINT) {
        (void)kill(worker, SIGTERM);
        _Exit(100);
    }
    if (close(hold[1]) != 0) {
        _Exit(101);
    }

    errno = ERANGE;
    if (pipe(hold) != 0 || errno != ERANGE) {
        _Exit(102);
    }

    errno = ERANGE;
    worker = fork();
    if (worker < 0) {
        _Exit(103);
    }
    if (worker == 0) {
        if (close(hold[1]) != 0) {
            _Exit(120);
        }
        if (read(hold[0], &command, 1U) != 1 || command != 'R') {
            _Exit(121);
        }

        errno = ERANGE;
        if (write(slave, resume_marker, sizeof(resume_marker) - 1U) !=
                (ssize_t)(sizeof(resume_marker) - 1U) ||
            errno != ERANGE) {
            _Exit(122);
        }

        if (close(hold[0]) != 0 || close(slave) != 0) {
            _Exit(123);
        }
        _Exit(53);
    }

    if (errno != ERANGE || worker <= 0 ||
        close(hold[0]) != 0) {
        _Exit(104);
    }

    errno = ERANGE;
    if (setpgid(worker, worker) != 0 || errno != ERANGE ||
        tcsetpgrp(slave, worker) != 0 || errno != ERANGE ||
        tcgetpgrp(slave) != worker || errno != ERANGE ||
        !write_phase(phase_write, 'S')) {
        (void)kill(worker, SIGTERM);
        (void)waitpid(worker, &status, 0);
        _Exit(105);
    }

    status = -1;
    errno = ERANGE;
    waited = waitpid(worker, &status, WUNTRACED);
    if (waited != worker || errno != ERANGE ||
        !WIFSTOPPED(status) || WSTOPSIG(status) != SIGTSTP) {
        (void)kill(worker, SIGTERM);
        (void)waitpid(worker, &status, 0);
        _Exit(106);
    }

    errno = ERANGE;
    if (kill(worker, SIGCONT) != 0 || errno != ERANGE) {
        (void)kill(worker, SIGTERM);
        (void)waitpid(worker, &status, 0);
        _Exit(107);
    }

    command = 'R';
    errno = ERANGE;
    if (write(hold[1], &command, 1U) != 1 ||
        close(hold[1]) != 0 ||
        errno != ERANGE) {
        (void)kill(worker, SIGTERM);
        (void)waitpid(worker, &status, 0);
        _Exit(108);
    }

    status = -1;
    errno = ERANGE;
    waited = waitpid(worker, &status, 0);
    if (waited != worker || errno != ERANGE ||
        !WIFEXITED(status) || WEXITSTATUS(status) != 53) {
        _Exit(109);
    }

    errno = ERANGE;
    if (tcsetpgrp(slave, self) != 0 || errno != ERANGE ||
        tcsetattr(slave, TCSANOW, &original) != 0 || errno != ERANGE ||
        close(slave) != 0 || errno != ERANGE ||
        close(phase_write) != 0 || errno != ERANGE) {
        _Exit(124);
    }

    _Exit(54);
}

int main(void)
{
    static const char ok[] = "tty-control-signals-ok\n";
    struct pollfd ready;
    char buffer[sizeof(resume_marker)];
    char phase;
    char intr = 3;
    char susp = 26;
    int phase_pipe[2];
    int unlock = 0;
    int master;
    int status;
    int count;
    pid_t child;

    errno = ERANGE;
    master = open("/dev/ptmx", O_RDWR | O_CLOEXEC | O_NOCTTY);
    if (master < 0 || errno != ERANGE) {
        return 1;
    }

    errno = ERANGE;
    if (mini_sys_ioctl(master, MINI_TIOCSPTLCK,
                       (unsigned long)&unlock) < 0L ||
        errno != ERANGE ||
        pipe(phase_pipe) != 0 ||
        errno != ERANGE) {
        close(master);
        return 2;
    }

    errno = ERANGE;
    child = fork();
    if (child < 0) {
        close(phase_pipe[0]);
        close(phase_pipe[1]);
        close(master);
        return 3;
    }
    if (child == 0) {
        if (close(phase_pipe[0]) != 0) {
            _Exit(80);
        }
        session_child(master, phase_pipe[1]);
    }

    if (errno != ERANGE || child <= 0 ||
        close(phase_pipe[1]) != 0) {
        close(phase_pipe[0]);
        close(master);
        return 4;
    }

    errno = ERANGE;
    if (read(phase_pipe[0], &phase, 1U) != 1 ||
        phase != 'I' || errno != ERANGE ||
        write(master, &intr, 1U) != 1 ||
        errno != ERANGE) {
        (void)kill(child, SIGTERM);
        (void)waitpid(child, &status, 0);
        close(phase_pipe[0]);
        close(master);
        return 5;
    }

    errno = ERANGE;
    if (read(phase_pipe[0], &phase, 1U) != 1 ||
        phase != 'S' || errno != ERANGE ||
        write(master, &susp, 1U) != 1 ||
        errno != ERANGE) {
        (void)kill(child, SIGTERM);
        (void)waitpid(child, &status, 0);
        close(phase_pipe[0]);
        close(master);
        return 6;
    }

    ready.fd = master;
    ready.events = POLLIN;
    ready.revents = 0;
    errno = ERANGE;
    if (poll(&ready, 1U, -1) != 1 || errno != ERANGE ||
        (ready.revents & POLLIN) == 0 ||
        (ready.revents & POLLNVAL) != 0) {
        (void)kill(child, SIGTERM);
        (void)waitpid(child, &status, 0);
        close(phase_pipe[0]);
        close(master);
        return 7;
    }

    errno = ERANGE;
    count = (int)read(master, buffer, sizeof(resume_marker) - 1U);
    if (count != (int)(sizeof(resume_marker) - 1U) ||
        errno != ERANGE ||
        !same_bytes(buffer, resume_marker, sizeof(resume_marker) - 1U)) {
        (void)kill(child, SIGTERM);
        (void)waitpid(child, &status, 0);
        close(phase_pipe[0]);
        close(master);
        return 8;
    }

    status = -1;
    errno = ERANGE;
    if (waitpid(child, &status, 0) != child ||
        errno != ERANGE ||
        !WIFEXITED(status) ||
        WEXITSTATUS(status) != 54) {
        close(phase_pipe[0]);
        close(master);
        return 9;
    }

    errno = ERANGE;
    if (read(phase_pipe[0], &phase, 1U) != 0 ||
        close(phase_pipe[0]) != 0 ||
        close(master) != 0 ||
        errno != ERANGE) {
        return 10;
    }

    if (mini_sys_write(STDOUT_FILENO, ok, sizeof(ok) - 1U) !=
        (long)(sizeof(ok) - 1U)) {
        return 11;
    }
    return 0;
}
