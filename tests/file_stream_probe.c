#include <errno.h>
#include <mini/syscall.h>
#include <stdio.h>

static int file_equals(const char *path, const char *expected, size_t length)
{
    FILE *stream = fopen(path, "r");
    size_t i;

    if (stream == (FILE *)0) {
        return 0;
    }
    for (i = 0; i < length; ++i) {
        if (fgetc(stream) != (unsigned char)expected[i]) {
            fclose(stream);
            return 0;
        }
    }
    if (fgetc(stream) != EOF || !feof(stream) || fclose(stream) != 0) {
        return 0;
    }
    return 1;
}

int main(int argc, char **argv)
{
    static const char ok[] = "file-stream-ok\n";
    char full_buffer[2];
    char line_buffer[8];
    char setbuf_storage[BUFSIZ];
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
    if (stream == (FILE *)0 ||
        setvbuf(stream, full_buffer, _IOFBF, sizeof(full_buffer)) != 0) {
        return 6;
    }
    if (fputc('A', stream) != 'A' || full_buffer[0] != 'A' ||
        fputc('B', stream) != 'B' || !file_equals(argv[1], "", 0)) {
        fclose(stream);
        return 7;
    }
    if (fputc('C', stream) != 'C' || !file_equals(argv[1], "AB", 2) ||
        fflush(stream) != 0 || !file_equals(argv[1], "ABC", 3) ||
        fclose(stream) != 0) {
        return 8;
    }

    stream = fopen(argv[1], "a");
    if (stream == (FILE *)0 ||
        setvbuf(stream, line_buffer, _IOLBF, sizeof(line_buffer)) != 0) {
        return 9;
    }
    if (fputc('D', stream) != 'D' || !file_equals(argv[1], "ABC", 3) ||
        fputc('\n', stream) != '\n' || !file_equals(argv[1], "ABCD\n", 5) ||
        fclose(stream) != 0) {
        return 10;
    }

    stream = fopen(argv[1], "wb");
    if (stream == (FILE *)0) {
        return 11;
    }
    setbuf(stream, setbuf_storage);
    if (fputs("AB", stream) == EOF || !file_equals(argv[1], "", 0) ||
        fclose(stream) != 0 || !file_equals(argv[1], "AB", 2)) {
        return 12;
    }

    stream = fopen(argv[1], "a");
    if (stream == (FILE *)0 || setvbuf(stream, (char *)0, _IOFBF, 4U) != 0 ||
        setvbuf(stream, (char *)0, _IONBF, 0U) != 0 ||
        fputc('C', stream) != 'C' || !file_equals(argv[1], "ABC", 3) ||
        fclose(stream) != 0) {
        return 13;
    }

    stream = fopen(argv[1], "r");
    if (stream == (FILE *)0) {
        return 14;
    }
    if (fgetc(stream) != 'A' || fgetc(stream) != 'B' ||
        fgetc(stream) != 'C' || fgetc(stream) != EOF ||
        !feof(stream) || ferror(stream)) {
        fclose(stream);
        return 15;
    }
    if (fclose(stream) != 0) {
        return 16;
    }

    errno = ERANGE;
    if (fopen(argv[1], "r++") != (FILE *)0 || errno != EINVAL) {
        return 17;
    }

    if (mini_sys_write(1, ok, sizeof(ok) - 1) != (long)(sizeof(ok) - 1)) {
        return 18;
    }
    return 0;
}
