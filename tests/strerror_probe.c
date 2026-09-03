#include <errno.h>
#include <mini/syscall.h>
#include <stddef.h>
#include <string.h>

static int same_string(const char *left, const char *right)
{
    size_t i = 0;

    while (left[i] != '\0' && right[i] != '\0') {
        if (left[i] != right[i]) {
            return 0;
        }
        ++i;
    }
    return left[i] == right[i];
}

int main(int argc, char **argv, char **envp)
{
    static const char ok[] = "strerror-ok\n";
    char *eio;
    char *unknown;

    (void)argc;
    (void)argv;
    (void)envp;

    errno = ERANGE;

    eio = strerror(EIO);
    if (eio == (char *)0 || !same_string(eio, "Input/output error") ||
        errno != ERANGE) {
        return 1;
    }
    if (!same_string(strerror(ENOMEM), "Cannot allocate memory") ||
        !same_string(strerror(EINVAL), "Invalid argument") ||
        !same_string(strerror(ERANGE), "Numerical result out of range") ||
        errno != ERANGE) {
        return 2;
    }

    unknown = strerror(0);
    if (unknown == (char *)0 || !same_string(unknown, "Unknown error") ||
        strerror(-1) != unknown || strerror(999999) != unknown ||
        errno != ERANGE) {
        return 3;
    }

    if (strerror(EIO) != eio || !same_string(eio, "Input/output error") ||
        !same_string(unknown, "Unknown error") || errno != ERANGE) {
        return 4;
    }

    if (mini_sys_write(1, ok, sizeof(ok) - 1) != (long)(sizeof(ok) - 1)) {
        return 5;
    }
    return 0;
}
