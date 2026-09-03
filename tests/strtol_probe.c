#include <errno.h>
#include <mini/syscall.h>
#include <stdlib.h>

static int check_parse(const char *text, int base, long expected,
                       long expected_offset, int expected_errno)
{
    char *end = (char *)0;
    long value;

    errno = 7;
    value = strtol(text, &end, base);
    if (value != expected) {
        return 0;
    }
    if ((long)(end - text) != expected_offset) {
        return 0;
    }
    if (errno != expected_errno) {
        return 0;
    }
    return 1;
}

int main(int argc, char **argv, char **envp)
{
    static const char message[] = "strtol-ok\n";
    static const char no_digits[] = "  +x";
    char invalid_sentinel;
    char *end;

    (void)argc;
    (void)argv;
    (void)envp;

    if (!check_parse("42", 10, 42, 2, 7)) return 1;
    if (!check_parse("  \t-42tail", 10, -42, 6, 7)) return 2;
    if (!check_parse("077", 0, 63, 3, 7)) return 3;
    if (!check_parse("0x1fZ", 0, 31, 4, 7)) return 4;
    if (!check_parse("0X2A!", 16, 42, 4, 7)) return 5;
    if (!check_parse("10102", 2, 10, 4, 7)) return 6;
    if (!check_parse("zZ", 36, 1295, 2, 7)) return 7;
    if (!check_parse("0x", 0, 0, 1, 7)) return 8;
    if (!check_parse("0xg", 16, 0, 1, 7)) return 9;
    if (!check_parse("09", 0, 0, 1, 7)) return 10;
    if (!check_parse("9223372036854775807", 10, __LONG_MAX__, 19, 7)) return 11;
    if (!check_parse("-9223372036854775808", 10, -__LONG_MAX__ - 1L, 20, 7)) return 12;
    if (!check_parse("9223372036854775808x", 10, __LONG_MAX__, 19, ERANGE)) return 13;
    if (!check_parse("-9223372036854775809x", 10, -__LONG_MAX__ - 1L, 20, ERANGE)) return 14;
    if (!check_parse("999999999999999999999999tail", 10, __LONG_MAX__, 24, ERANGE)) return 15;

    end = (char *)0;
    errno = 7;
    if (strtol(no_digits, &end, 10) != 0 || end != no_digits || errno != 7) {
        return 16;
    }

    end = &invalid_sentinel;
    errno = 7;
    if (strtol("123", &end, 1) != 0 || end != &invalid_sentinel || errno != EINVAL) {
        return 17;
    }

    end = &invalid_sentinel;
    errno = 7;
    if (strtol("123", &end, 37) != 0 || end != &invalid_sentinel || errno != EINVAL) {
        return 18;
    }

    errno = 7;
    if (strtol("123", (char **)0, 10) != 123 || errno != 7) {
        return 19;
    }

    if (mini_sys_write(1, message, sizeof(message) - 1) !=
        (long)(sizeof(message) - 1)) {
        return 20;
    }
    return 0;
}
