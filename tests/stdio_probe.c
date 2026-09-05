#include <errno.h>
#include <mini/syscall.h>
#include <stdio.h>

int main(void)
{
    static const char expected_basic[] =
        "fmt:-42:17:4000000000:11:2a:2A:Z:ok:%\n";
    static const char expected_padding[] =
        "pad:[-00042][xy   ][0x2a][011][abc][    0023][  0007]\n";
    static const char expected_lengths[] =
        "len:-5:250:-30000:60000:-1234567890:4000000000:-5000000000:9000000000\n";
    static const char expected_star[] =
        "star:[12   ][wide][    002a]\n";
    static const char expected_signs[] =
        "sign:[+7][ 7][00000042][0X2A]\n";
    static const char expected_zero_precision[] =
        "edge:[][0][     ]\n";
    static const char expected_minima[] =
        "min:-2147483648:-9223372036854775808\n";
    static const char expected_fprintf_stack[] =
        "fprintf-stack:1:2:3:4:5\n";
    int result;

    if (stdin == (FILE *)0 || stdout == (FILE *)0 || stderr == (FILE *)0 ||
        stdin == stdout || stdin == stderr || stdout == stderr) {
        return 1;
    }

    errno = ERANGE;
    result = fgetc(stdin);
    if (result != 'x' || errno != ERANGE || feof(stdin) || ferror(stdin)) {
        return 2;
    }

    result = getchar();
    if (result != 'y' || errno != ERANGE || feof(stdin) || ferror(stdin)) {
        return 3;
    }

    result = getc(stdin);
    if (result != EOF || !feof(stdin) || ferror(stdin) || errno != ERANGE) {
        return 4;
    }
    clearerr(stdin);
    if (feof(stdin) || ferror(stdin)) {
        return 5;
    }

    errno = ERANGE;
    result = fgetc(stdout);
    if (result != EOF || !ferror(stdout) || feof(stdout) || errno != EINVAL) {
        return 6;
    }
    clearerr(stdout);
    if (ferror(stdout) || feof(stdout)) {
        return 7;
    }

    errno = ERANGE;
    result = fputc('!', stdin);
    if (result != EOF || !ferror(stdin) || feof(stdin) || errno != EINVAL) {
        return 8;
    }
    clearerr(stdin);

    errno = ERANGE;
    if (fputc('A', stdout) != 'A' || putc(0x142, stdout) != 'B' ||
        putchar('C') != 'C' || fputs("DE", stdout) < 0 ||
        puts("FG") < 0 || errno != ERANGE || ferror(stdout)) {
        return 9;
    }

    if (fputs("stderr-ok", stderr) < 0 || fputc('\n', stderr) != '\n' ||
        errno != ERANGE || ferror(stderr)) {
        return 10;
    }

    errno = ERANGE;
    result = printf("fmt:%d:%i:%u:%o:%x:%X:%c:%s:%%\n",
                    -42, 17, 4000000000U, 9U, 0x2aU, 0x2aU, 'Z', "ok");
    if (result != (int)(sizeof(expected_basic) - 1U) || errno != ERANGE ||
        ferror(stdout)) {
        return 11;
    }

    result = printf("pad:[%+06d][%-5s][%#x][%#o][%.3s][%8.4d][%*.*u]\n",
                    -42, "xy", 0x2aU, 9U, "abcdef", 23, 6, 4, 7U);
    if (result != (int)(sizeof(expected_padding) - 1U) || errno != ERANGE ||
        ferror(stdout)) {
        return 12;
    }

    result = printf("len:%hhd:%hhu:%hd:%hu:%ld:%lu:%lld:%llu\n",
                    -5, 250, -30000, 60000,
                    -1234567890L, 4000000000UL,
                    -5000000000LL, 9000000000ULL);
    if (result != (int)(sizeof(expected_lengths) - 1U) || errno != ERANGE ||
        ferror(stdout)) {
        return 13;
    }

    result = printf("star:[%*d][%.*s][%0*.*x]\n",
                    -5, 12, -1, "wide", 8, 4, 0x2aU);
    if (result != (int)(sizeof(expected_star) - 1U) || errno != ERANGE ||
        ferror(stdout)) {
        return 14;
    }

    result = printf("sign:[%+d][% d][%08u][%#X]\n", 7, 7, 42U, 0x2aU);
    if (result != (int)(sizeof(expected_signs) - 1U) || errno != ERANGE ||
        ferror(stdout)) {
        return 15;
    }

    result = printf("edge:[%.0d][%#.0o][%5.0u]\n", 0, 0U, 0U);
    if (result != (int)(sizeof(expected_zero_precision) - 1U) ||
        errno != ERANGE || ferror(stdout)) {
        return 16;
    }

    result = printf("min:%d:%lld\n",
                    (-2147483647 - 1), (-9223372036854775807LL - 1LL));
    if (result != (int)(sizeof(expected_minima) - 1U) || errno != ERANGE ||
        ferror(stdout)) {
        return 17;
    }

    result = fprintf(stderr, "format-err:%#08x:%-4c\n", 0x2aU, 'Q');
    if (result != 25 || errno != ERANGE || ferror(stderr)) {
        return 18;
    }

    result = fprintf(stderr, "fprintf-stack:%d:%d:%d:%d:%d\n", 1, 2, 3, 4, 5);
    if (result != (int)(sizeof(expected_fprintf_stack) - 1U) ||
        errno != ERANGE || ferror(stderr)) {
        return 19;
    }

    errno = ERANGE;
    if (printf("%q") != EOF || errno != EINVAL || ferror(stdout)) {
        return 20;
    }
    errno = ERANGE;
    if (printf("%") != EOF || errno != EINVAL || ferror(stdout)) {
        return 21;
    }
    errno = ERANGE;
    if (printf("%*s", (-2147483647 - 1), "x") != EOF ||
        errno != EINVAL || ferror(stdout)) {
        return 22;
    }

    errno = ERANGE;
    if (fflush(stdout) == EOF || errno != ERANGE || ferror(stdout)) {
        return 23;
    }

    {
        static const char marker[] = "stdio-ok";
        long written = mini_sys_write(1, marker, sizeof(marker) - 1);

        if (written != (long)(sizeof(marker) - 1)) {
            return 24;
        }
    }

    if (mini_sys_close(1) != 0) {
        return 25;
    }
    clearerr(stdout);
    errno = ERANGE;
    if (printf("%300s", "x") != EOF || !ferror(stdout)) {
        return 26;
    }

    return 0;
}
