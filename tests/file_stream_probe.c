#include <errno.h>
#include <mini/syscall.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    static const char ok[] = "file-stream-ok\n";
    FILE *stream;

    if (argc != 2) {
        return 1;
    }

    errno = ERANGE;
    stream = fopen(argv[1], "r");
    if (stream != (FILE *)0 || errno != ENOENT) {
        if (stream != (FILE *)0) {
            fclose(stream);
        }
        return 2;
    }

    errno = ERANGE;
    stream = fopen(argv[1], "w");
    if (stream == (FILE *)0 || errno != ERANGE) {
        return 3;
    }
    if (fputs("OLD", stream) == EOF || ferror(stream) || errno != ERANGE) {
        fclose(stream);
        return 4;
    }
    if (fclose(stream) != 0 || errno != ERANGE) {
        return 5;
    }

    stream = fopen(argv[1], "wb");
    if (stream == (FILE *)0) {
        return 6;
    }
    if (fputs("AB", stream) == EOF || fclose(stream) != 0) {
        return 7;
    }

    stream = fopen(argv[1], "a");
    if (stream == (FILE *)0) {
        return 8;
    }
    if (fseek(stream, 0L, SEEK_SET) != 0 || fputc('C', stream) != 'C' ||
        ftell(stream) != 3L || fclose(stream) != 0) {
        return 9;
    }

    stream = fopen(argv[1], "r");
    if (stream == (FILE *)0) {
        return 10;
    }
    if (fgetc(stream) != 'A' || fgetc(stream) != 'B' ||
        fgetc(stream) != 'C' || fgetc(stream) != EOF ||
        !feof(stream) || ferror(stream)) {
        fclose(stream);
        return 11;
    }
    if (fclose(stream) != 0) {
        return 12;
    }

    errno = ERANGE;
    if (fopen(argv[1], "r++") != (FILE *)0 || errno != EINVAL) {
        return 13;
    }

    if (mini_sys_write(1, ok, sizeof(ok) - 1) != (long)(sizeof(ok) - 1)) {
        return 14;
    }
    return 0;
}
