#include <errno.h>
#include <mini/syscall.h>
#include <stdio.h>

int main(void)
{
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

    {
        static const char marker[] = "stdio-ok";
        long written = mini_sys_write(1, marker, sizeof(marker) - 1);

        if (written != (long)(sizeof(marker) - 1)) {
            return 11;
        }
    }

    return 0;
}
