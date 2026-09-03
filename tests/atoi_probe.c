#include <mini/syscall.h>
#include <stdlib.h>

struct atoi_case {
    const char *input;
    int expected;
};

int main(int argc, char **argv, char **envp)
{
    static const struct atoi_case cases[] = {
        {"", 0}, {"   ", 0}, {"abc", 0}, {"0", 0}, {"42", 42},
        {"-17", -17}, {"+19", 19}, {" \t\n\v\f\r  73tail", 73},
        {"00123x", 123}, {"\t-00123xyz", -123}, {"-0", 0},
        {"+ 1", 0}, {"--1", 0}, {"2147483647", __INT_MAX__},
        {"-2147483648", -__INT_MAX__ - 1},
        {"2147483648", __INT_MAX__},
        {"999999999999999999999999", __INT_MAX__},
        {"-2147483649", -__INT_MAX__ - 1},
        {"-999999999999999999999999", -__INT_MAX__ - 1},
    };
    static const char ok[] = "atoi-ok\n";
    unsigned long i;

    (void)argc;
    (void)argv;
    (void)envp;

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        if (atoi(cases[i].input) != cases[i].expected) {
            return 40 + (int)i;
        }
    }

    if (mini_sys_write(1, ok, sizeof(ok) - 1) != (long)(sizeof(ok) - 1)) {
        return 70;
    }
    return 0;
}
