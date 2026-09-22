#include <errno.h>
#include <fcntl.h>
#include <mini/syscall.h>
#include <mini/tty_ioctl.h>
#include <poll.h>
#include <stddef.h>
#include <termios.h>
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
    static const char canonical_input[] = "abc\n";
    static const char ok[] = "termios-canonical-ok\n";
    struct pollfd ready;
    struct termios original;
    struct termios canonical;
    struct termios noncanonical;
    struct termios observed;
    char buffer[8];
    int pipefd[2];
    int unlock = 0;
    int master;
    int slave;
    int result;

    errno = ERANGE;
    if (pipe(pipefd) != 0 || errno != ERANGE) {
        return 1;
    }

    errno = ERANGE;
    if (tcgetattr(pipefd[0], &observed) != -1 || errno != ENOTTY) {
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
    slave = (int)mini_sys_ioctl(master, MINI_TIOCGPTPEER,
                                (unsigned long)(O_RDWR | O_CLOEXEC |
                                                O_NOCTTY));
    if (slave < 0 || errno != ERANGE) {
        close(master);
        return 6;
    }

    errno = ERANGE;
    if (tcgetattr(slave, &original) != 0 || errno != ERANGE) {
        close(slave);
        close(master);
        return 7;
    }

    canonical = original;
    canonical.c_lflag |= ICANON;
    canonical.c_lflag &= ~ECHO;

    errno = ERANGE;
    if (tcsetattr(slave, TCSANOW, &canonical) != 0 ||
        errno != ERANGE ||
        tcgetattr(slave, &observed) != 0 ||
        errno != ERANGE ||
        (observed.c_lflag & ICANON) == 0U ||
        (observed.c_lflag & ECHO) != 0U) {
        close(slave);
        close(master);
        return 8;
    }

    errno = ERANGE;
    if (write(master, "abc", 3U) != 3 || errno != ERANGE) {
        close(slave);
        close(master);
        return 9;
    }

    ready.fd = slave;
    ready.events = POLLIN;
    ready.revents = 0;
    errno = ERANGE;
    result = poll(&ready, 1U, 0);
    if (result != 0 || errno != ERANGE || ready.revents != 0) {
        close(slave);
        close(master);
        return 10;
    }

    errno = ERANGE;
    if (write(master, "\n", 1U) != 1 || errno != ERANGE) {
        close(slave);
        close(master);
        return 11;
    }

    ready.fd = slave;
    ready.events = POLLIN;
    ready.revents = 0;
    errno = ERANGE;
    result = poll(&ready, 1U, -1);
    if (result != 1 || errno != ERANGE ||
        (ready.revents & POLLIN) == 0 ||
        (ready.revents & POLLNVAL) != 0) {
        close(slave);
        close(master);
        return 12;
    }

    errno = ERANGE;
    if (read(slave, buffer, sizeof(canonical_input) - 1U) !=
            (ssize_t)(sizeof(canonical_input) - 1U) ||
        errno != ERANGE ||
        !same_bytes(buffer, canonical_input,
                    sizeof(canonical_input) - 1U)) {
        close(slave);
        close(master);
        return 13;
    }

    noncanonical = canonical;
    noncanonical.c_lflag &= ~ICANON;
    noncanonical.c_cc[VMIN] = 1U;
    noncanonical.c_cc[VTIME] = 0U;

    errno = ERANGE;
    if (tcsetattr(slave, TCSANOW, &noncanonical) != 0 ||
        errno != ERANGE ||
        tcgetattr(slave, &observed) != 0 ||
        errno != ERANGE ||
        (observed.c_lflag & ICANON) != 0U ||
        observed.c_cc[VMIN] != 1U ||
        observed.c_cc[VTIME] != 0U) {
        close(slave);
        close(master);
        return 14;
    }

    errno = ERANGE;
    if (write(master, "Z", 1U) != 1 || errno != ERANGE) {
        close(slave);
        close(master);
        return 15;
    }

    ready.fd = slave;
    ready.events = POLLIN;
    ready.revents = 0;
    errno = ERANGE;
    result = poll(&ready, 1U, -1);
    if (result != 1 || errno != ERANGE ||
        (ready.revents & POLLIN) == 0) {
        close(slave);
        close(master);
        return 16;
    }

    errno = ERANGE;
    if (read(slave, buffer, 1U) != 1 ||
        buffer[0] != 'Z' ||
        errno != ERANGE) {
        close(slave);
        close(master);
        return 17;
    }

    errno = ERANGE;
    if (tcsetattr(slave, 99, &original) != -1 || errno != EINVAL) {
        close(slave);
        close(master);
        return 18;
    }

    errno = ERANGE;
    if (tcsetattr(slave, TCSANOW, &original) != 0 ||
        errno != ERANGE ||
        close(slave) != 0 ||
        errno != ERANGE ||
        close(master) != 0 ||
        errno != ERANGE) {
        return 19;
    }

    if (mini_sys_write(STDOUT_FILENO, ok, sizeof(ok) - 1U) !=
        (long)(sizeof(ok) - 1U)) {
        return 20;
    }
    return 0;
}
