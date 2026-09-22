#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int same_string(const char *left, const char *right)
{
    while (*left != '\0' && *left == *right) {
        ++left;
        ++right;
    }
    return *left == *right;
}

int main(int argc, char **argv, char **envp)
{
    static const char message[] = "exec-child-ok\n";
    char *token;

    (void)envp;

    if (argc != 3 ||
        !same_string(argv[1], "alpha") ||
        !same_string(argv[2], "beta")) {
        return 31;
    }

    token = getenv("MINI_EXEC_TOKEN");
    if (token == (char *)0 || !same_string(token, "launch-ok")) {
        return 32;
    }

    errno = ERANGE;
    if (fcntl(100, F_GETFD) != -1 || errno != EBADF) {
        return 33;
    }

    errno = ERANGE;
    if (write(STDOUT_FILENO, message, sizeof(message) - 1U) !=
            (ssize_t)(sizeof(message) - 1U) ||
        errno != ERANGE) {
        return 34;
    }

    return 37;
}
