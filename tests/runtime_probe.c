#include <mini/syscall.h>
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

static void marker_a(void)
{
    write_marker('A');
}

static void marker_b(void)
{
    write_marker('B');
}

static void marker_c(void)
{
    write_marker('C');
}

static void marker_x(void)
{
    write_marker('X');
}

static void buffered_b(void)
{
    if (fputc('B', stdout) != 'B') {
        _Exit(94);
    }
}

static void register_three(void)
{
    if (atexit(marker_a) != 0 || atexit(marker_b) != 0 || atexit(marker_c) != 0) {
        _Exit(91);
    }
}

static int run_termination_probe(const char *mode)
{
    unsigned int i;

    if (equals(mode, "return-exit")) {
        register_three();
        return 23;
    }

    if (equals(mode, "call-exit")) {
        register_three();
        exit(24);
    }

    if (equals(mode, "quick-exit")) {
        register_three();
        _Exit(25);
    }

    if (equals(mode, "capacity")) {
        for (i = 0; i < 32U; ++i) {
            if (atexit(marker_x) != 0) {
                _Exit(92);
            }
        }
        if (atexit(marker_x) == 0) {
            _Exit(93);
        }
        return 26;
    }

    if (equals(mode, "buffered-return")) {
        if (atexit(buffered_b) != 0 || fputc('A', stdout) != 'A') {
            _Exit(95);
        }
        return 27;
    }

    if (equals(mode, "buffered-call")) {
        if (atexit(buffered_b) != 0 || fputc('A', stdout) != 'A') {
            _Exit(96);
        }
        exit(28);
    }

    if (equals(mode, "buffered-quick")) {
        if (fputc('A', stdout) != 'A') {
            _Exit(97);
        }
        _Exit(29);
    }

    if (equals(mode, "buffered-flush-quick")) {
        if (fputc('A', stdout) != 'A' || fflush(stdout) == EOF) {
            _Exit(98);
        }
        _Exit(30);
    }

    return -1;
}

int main(int argc, char **argv, char **envp)
{
    static const char ok[] = "runtime-ok\n";
    int saw_sentinel = 0;
    char **entry;

    if (argc == 2 && equals(argv[0], "./build/runtime_probe")) {
        int status = run_termination_probe(argv[1]);

        if (status >= 0) {
            return status;
        }
        return 13;
    }

    if (argc != 3 || !equals(argv[0], "./build/runtime_probe") ||
        !equals(argv[1], "alpha") || !equals(argv[2], "beta") ||
        argv[3] != (char *)0) {
        return 10;
    }

    for (entry = envp; *entry != (char *)0; ++entry) {
        if (equals(*entry, "MINI_LIBC_SENTINEL=present")) {
            saw_sentinel = 1;
            break;
        }
    }
    if (!saw_sentinel) {
        return 11;
    }

    if (mini_sys_write(1, ok, sizeof(ok) - 1) != (long)(sizeof(ok) - 1)) {
        return 12;
    }
    return 37;
}
