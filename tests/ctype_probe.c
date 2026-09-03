#include <ctype.h>
#include <errno.h>
#include <mini/syscall.h>
#include <stdio.h>

static int model_digit(int c)
{
    return c >= '0' && c <= '9';
}

static int model_space(int c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' ||
           c == '\f' || c == '\r';
}

int main(int argc, char **argv, char **envp)
{
    static const char ok[] = "ctype-ok\n";
    int c;

    (void)argc;
    (void)argv;
    (void)envp;

    errno = ERANGE;
    if (isdigit(EOF) != 0 || isspace(EOF) != 0 || errno != ERANGE) {
        return 1;
    }

    for (c = 0; c <= 255; ++c) {
        if ((isdigit(c) != 0) != model_digit(c)) {
            return 2;
        }
        if ((isspace(c) != 0) != model_space(c)) {
            return 3;
        }
        if (errno != ERANGE) {
            return 4;
        }
    }

    if (mini_sys_write(1, ok, sizeof(ok) - 1) != (long)(sizeof(ok) - 1)) {
        return 5;
    }
    return 0;
}
