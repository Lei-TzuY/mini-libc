#include <errno.h>
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

int main(int argc, char **argv)
{
    static const char message[] = "spawn-child-ok\n";
    char *token;

    if (argc != 3 ||
        !same_string(argv[1], "gamma") ||
        !same_string(argv[2], "delta")) {
        return 41;
    }

    token = getenv("MINI_SPAWN_TOKEN");
    if (token == (char *)0 || !same_string(token, "threaded-ok")) {
        return 42;
    }

    errno = ERANGE;
    if (write(STDOUT_FILENO, message, sizeof(message) - 1U) !=
            (ssize_t)(sizeof(message) - 1U) ||
        errno != ERANGE) {
        return 43;
    }

    return 44;
}
