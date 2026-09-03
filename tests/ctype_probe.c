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

static int model_graph(int c)
{
    return c >= 0x21 && c <= 0x7e;
}

static int model_lower(int c)
{
    return c >= 'a' && c <= 'z';
}

static int model_print(int c)
{
    return c >= 0x20 && c <= 0x7e;
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

static int model_tolower(int c)
{
    return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c;
}

static int model_toupper(int c)
{
    return c >= 'a' && c <= 'z' ? c - ('a' - 'A') : c;
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
        isgraph(EOF) != 0 || islower(EOF) != 0 || isprint(EOF) != 0 ||
        isspace(EOF) != 0 || isupper(EOF) != 0 || tolower(EOF) != EOF ||
        toupper(EOF) != EOF || errno != ERANGE) {
        return 1;
    }

    if (isprint(' ') == 0 || isgraph(' ') != 0) {
        return 2;
    }

    for (c = 0; c <= 255; ++c) {
        if ((isalpha(c) != 0) != model_alpha(c)) {
            return 3;
        }
        if ((isalnum(c) != 0) != model_alnum(c)) {
            return 4;
        }
        if ((isdigit(c) != 0) != model_digit(c)) {
            return 5;
        }
        if ((isgraph(c) != 0) != model_graph(c)) {
            return 6;
        }
        if ((islower(c) != 0) != model_lower(c)) {
            return 7;
        }
        if ((isprint(c) != 0) != model_print(c)) {
            return 8;
        }
        if ((isspace(c) != 0) != model_space(c)) {
            return 9;
        }
        if ((isupper(c) != 0) != model_upper(c)) {
            return 10;
        }
        if (tolower(c) != model_tolower(c)) {
            return 11;
        }
        if (toupper(c) != model_toupper(c)) {
            return 12;
        }
        if (errno != ERANGE) {
            return 13;
        }
    }

    if (mini_sys_write(1, ok, sizeof(ok) - 1) != (long)(sizeof(ok) - 1)) {
        return 14;
    }
    return 0;
}
