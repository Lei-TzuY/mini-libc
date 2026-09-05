#include <mini/syscall.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

static int equals(const char *left, const char *right)
{
    while (*left != '\0' && *left == *right) {
        ++left;
        ++right;
    }
    return *left == *right;
}

static void write_marker(char marker)
{
    if (mini_sys_write(1, &marker, 1) != 1) {
        _Exit(90);
    }
}

static void normal_handler(void)
{
    write_marker('N');
}

static void quick_1(void)
{
    write_marker('1');
}

static void quick_2(void)
{
    write_marker('2');
}

static void abort_handler(int sig)
{
    if (sig != SIGABRT) {
        _Exit(91);
    }
    write_marker('H');
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        return 1;
    }

    if (equals(argv[1], "quick")) {
        if (atexit(normal_handler) != 0 || at_quick_exit(quick_1) != 0 ||
            at_quick_exit(quick_2) != 0 || fputc('Z', stdout) != 'Z') {
            _Exit(92);
        }
        quick_exit(41);
    }

    if (equals(argv[1], "abort")) {
        if (atexit(normal_handler) != 0 || at_quick_exit(quick_1) != 0 ||
            signal(SIGABRT, abort_handler) == SIG_ERR ||
            fputc('Z', stdout) != 'Z') {
            _Exit(93);
        }
        abort();
    }

    return 2;
}
