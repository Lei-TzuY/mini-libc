#include <ctype.h>
#include <errno.h>
#include <mini/syscall.h>
#include <stdio.h>

static int model_alpha(int c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static int model_digit(int c)
{
    return c >= '0' && c <= '9';
}

static int model_alnum(int c)
{
    return model_alpha(c) || model_digit(c);
}

static int model_lower(int c)
{
    return c >= 'a' && c <= 'z';
}

static int model_space(int c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' ||
           c == '\f' || c == '\r';
}

static int model_upper(int c)
{
    return c >= 'A' && c <= 'Z';
}

int main(int argc, char **argv, char **envp)
{
    static const char ok[] = "ctype-ok\n";
    int c;

    (void)argc;
    (void)argv;
    (void)envp;

    errno = ERANGE;
    if (isalpha(EOF) != 0 || isalnum(EOF) != 0 || isdigit(EOF) != 0 ||
        islower(EOF) != 0 || isspace(EOF) != 0 || isupper(EOF) != 0 ||
        errno != ERANGE) {
        return 1;
    }

    for (c = 0; c <= 255; ++c) {
        if ((isalpha(c) != 0) != model_alpha(c)) {
            return 2;
        }
        if ((isalnum(c) != 0) != model_alnum(c)) {
            return 3;
        }
        if ((isdigit(c) != 0) != model_digit(c)) {
            return 4;
        }
        if ((islower(c) != 0) != model_lower(c)) {
            return 5;
        }
        if ((isspace(c) != 0) != model_space(c)) {
            return 6;
        }
        if ((isupper(c) != 0) != model_upper(c)) {
            return 7;
        }
        if (errno != ERANGE) {
            return 8;
        }
    }

    if (mini_sys_write(1, ok, sizeof(ok) - 1) != (long)(sizeof(ok) - 1)) {
        return 9;
    }
    return 0;
}
