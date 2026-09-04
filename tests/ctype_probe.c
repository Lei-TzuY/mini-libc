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

static int model_cntrl(int c)
{
    return (c >= 0x00 && c <= 0x1f) || c == 0x7f;
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

static int model_xdigit(int c)
{
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') ||
           (c >= 'a' && c <= 'f');
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
    if (isalpha(EOF) != 0 || isalnum(EOF) != 0 || iscntrl(EOF) != 0 ||
        isdigit(EOF) != 0 || isgraph(EOF) != 0 || islower(EOF) != 0 ||
        isprint(EOF) != 0 || isspace(EOF) != 0 || isupper(EOF) != 0 ||
        isxdigit(EOF) != 0 || tolower(EOF) != EOF || toupper(EOF) != EOF ||
        errno != ERANGE) {
        return 1;
    }

    if (iscntrl(0x00) == 0 || iscntrl(0x1f) == 0 || iscntrl(0x20) != 0 ||
        iscntrl(0x7f) == 0 || iscntrl(0x80) != 0) {
        return 2;
    }

    if (isprint(' ') == 0 || isgraph(' ') != 0) {
        return 3;
    }

    for (c = 0; c <= 255; ++c) {
        if ((isalpha(c) != 0) != model_alpha(c)) {
            return 4;
        }
        if ((isalnum(c) != 0) != model_alnum(c)) {
            return 5;
        }
        if ((iscntrl(c) != 0) != model_cntrl(c)) {
            return 6;
        }
        if ((isdigit(c) != 0) != model_digit(c)) {
            return 7;
        }
        if ((isgraph(c) != 0) != model_graph(c)) {
            return 8;
        }
        if ((islower(c) != 0) != model_lower(c)) {
            return 9;
        }
        if ((isprint(c) != 0) != model_print(c)) {
            return 10;
        }
        if ((isspace(c) != 0) != model_space(c)) {
            return 11;
        }
        if ((isupper(c) != 0) != model_upper(c)) {
            return 12;
        }
        if ((isxdigit(c) != 0) != model_xdigit(c)) {
            return 13;
        }
        if (tolower(c) != model_tolower(c)) {
            return 14;
        }
        if (toupper(c) != model_toupper(c)) {
            return 15;
        }
        if (errno != ERANGE) {
            return 16;
        }
    }

    if (mini_sys_write(1, ok, sizeof(ok) - 1) != (long)(sizeof(ok) - 1)) {
        return 17;
    }
    return 0;
}
