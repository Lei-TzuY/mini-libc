#include <errno.h>
#include <mini/syscall.h>
#include <stdlib.h>

static int equals(const char *left, const char *right)
{
    while (*left != '\0' && *left == *right) {
        ++left;
        ++right;
    }
    return *left == *right;
}

static char *find_value(char **envp, const char *entry_text, unsigned long offset)
{
    char **entry;

    for (entry = envp; *entry != (char *)0; ++entry) {
        if (equals(*entry, entry_text)) {
            return *entry + offset;
        }
    }
    return (char *)0;
}

int main(int argc, char **argv, char **envp)
{
    static const char ok[] = "getenv-ok\n";
    char *alpha_expected;
    char *empty_expected;
    char *value;

    (void)argc;
    (void)argv;

    alpha_expected = find_value(envp, "MINI_GETENV_ALPHA=value", 18UL);
    empty_expected = find_value(envp, "MINI_GETENV_EMPTY=", 18UL);
    if (alpha_expected == (char *)0 || empty_expected == (char *)0) {
        return 1;
    }

    errno = 73;
    value = getenv("MINI_GETENV_ALPHA");
    if (value != alpha_expected || !equals(value, "value") || errno != 73) {
        return 2;
    }

    value = getenv("MINI_GETENV_EMPTY");
    if (value != empty_expected || value == (char *)0 || *value != '\0' || errno != 73) {
        return 3;
    }

    if (getenv("MINI_GETENV_MISSING") != (char *)0 || errno != 73) {
        return 4;
    }
    if (getenv("MINI_GETENV") != (char *)0 || errno != 73) {
        return 5;
    }
    if (getenv("") != (char *)0 || errno != 73) {
        return 6;
    }
    if (getenv("MINI_GETENV_ALPHA=value") != (char *)0 || errno != 73) {
        return 7;
    }

    if (mini_sys_write(1, ok, sizeof(ok) - 1) != (long)(sizeof(ok) - 1)) {
        return 8;
    }
    return 0;
}
