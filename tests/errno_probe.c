#include <errno.h>
#include <mini/syscall.h>

int main(int argc, char **argv, char **envp)
{
    static const char message[] = "errno-ok\n";
    int *slot;

    (void)argc;
    (void)argv;
    (void)envp;

    if (ERANGE != 34) {
        return 1;
    }
    if (errno != 0) {
        return 2;
    }

    slot = &errno;
    if (slot != __mini_errno_location()) {
        return 3;
    }

    errno = ERANGE;
    if (errno != ERANGE || *slot != ERANGE) {
        return 4;
    }

    *slot = 7;
    if (errno != 7) {
        return 5;
    }

    errno = 0;
    if (mini_sys_write(1, message, sizeof(message) - 1) !=
        (long)(sizeof(message) - 1)) {
        return 6;
    }
    return 0;
}
