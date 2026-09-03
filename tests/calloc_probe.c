#include <errno.h>
#include <mini/syscall.h>
#include <stddef.h>
#include <stdlib.h>

static int all_zero(const unsigned char *p, size_t n)
{
    size_t i;

    for (i = 0; i < n; ++i) {
        if (p[i] != 0) {
            return 0;
        }
    }
    return 1;
}

int main(int argc, char **argv, char **envp)
{
    static const char ok[] = "calloc-ok\n";
    unsigned char *p;
    unsigned char *q;
    size_t i;
    size_t huge = (size_t)-1 / 2U + 1U;

    (void)argc;
    (void)argv;
    (void)envp;

    errno = 77;
    if (calloc(0, 64) != (void *)0 || errno != 77) {
        return 1;
    }
    if (calloc(64, 0) != (void *)0 || errno != 77) {
        return 2;
    }

    errno = 91;
    p = (unsigned char *)calloc(32, 4);
    if (p == (unsigned char *)0 || errno != 91) {
        return 3;
    }
    if (((__UINTPTR_TYPE__)p & 15U) != 0U || !all_zero(p, 128)) {
        return 4;
    }

    for (i = 0; i < 128; ++i) {
        p[i] = (unsigned char)(0xa0U + (i & 15U));
    }
    free(p);

    errno = 23;
    q = (unsigned char *)calloc(16, 8);
    if (q == (unsigned char *)0 || errno != 23) {
        return 5;
    }
    if (q != p || !all_zero(q, 128)) {
        return 6;
    }
    free(q);

    errno = 0;
    if (calloc(huge, 2) != (void *)0 || errno != ENOMEM) {
        return 7;
    }

    if (mini_sys_write(1, ok, sizeof(ok) - 1) != (long)(sizeof(ok) - 1)) {
        return 8;
    }
    return 0;
}
