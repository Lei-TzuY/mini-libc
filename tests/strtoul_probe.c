#include <errno.h>
#include <mini/syscall.h>
#include <stdlib.h>

static int check_parse(const char *text, int base, unsigned long expected,
                       long expected_offset, int expected_errno)
{
    char *end = (char *)0;
    unsigned long value;

    errno = 7;
    value = strtoul(text, &end, base);
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
    static const char message[] = "strtoul-ok\n";
    static const char no_digits[] = "  +x";
    char invalid_sentinel;
    char *end;

    (void)argc;
    (void)argv;
    (void)envp;

    if (!check_parse("42", 10, 42UL, 2, 7)) return 1;
    if (!check_parse("  \t+42tail", 10, 42UL, 6, 7)) return 2;
    if (!check_parse("077", 0, 63UL, 3, 7)) return 3;
    if (!check_parse("0x1fZ", 0, 31UL, 4, 7)) return 4;
    if (!check_parse("0X2A!", 16, 42UL, 4, 7)) return 5;
    if (!check_parse("10102", 2, 10UL, 4, 7)) return 6;
    if (!check_parse("zZ", 36, 1295UL, 2, 7)) return 7;
    if (!check_parse("0x", 0, 0UL, 1, 7)) return 8;
    if (!check_parse("0xg", 16, 0UL, 1, 7)) return 9;
    if (!check_parse("09", 0, 0UL, 1, 7)) return 10;
    if (!check_parse("18446744073709551615", 10, ~0UL, 20, 7)) return 11;
    if (!check_parse("18446744073709551616x", 10, ~0UL, 20, ERANGE)) return 12;
    if (!check_parse("-1", 10, ~0UL, 2, 7)) return 13;
    if (!check_parse("-18446744073709551615", 10, 1UL, 21, 7)) return 14;
    if (!check_parse("-18446744073709551616x", 10, ~0UL, 21, ERANGE)) return 15;
    if (!check_parse("999999999999999999999999tail", 10, ~0UL, 24, ERANGE)) return 16;

    end = (char *)0;
    errno = 7;
    if (strtoul(no_digits, &end, 10) != 0UL || end != no_digits || errno != 7) {
        return 17;
    }

    end = &invalid_sentinel;
    errno = 7;
    if (strtoul("123", &end, 1) != 0UL || end != &invalid_sentinel || errno != EINVAL) {
        return 18;
    }

    end = &invalid_sentinel;
    errno = 7;
    if (strtoul("123", &end, 37) != 0UL || end != &invalid_sentinel || errno != EINVAL) {
        return 19;
    }

    errno = 7;
    if (strtoul("123", (char **)0, 10) != 123UL || errno != 7) {
        return 20;
    }

    if (mini_sys_write(1, message, sizeof(message) - 1) !=
        (long)(sizeof(message) - 1)) {
        return 21;
    }
    return 0;
}
