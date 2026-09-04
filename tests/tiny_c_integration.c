#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv, char **envp)
{
    char *end;
    char *value;
    char *io_path;
    char *buffer;
    FILE *stream;

    (void)envp;

    if (argc != 2 || strcmp(argv[1], "arg") != 0) {
        return 1;
    }

    value = getenv("MINI_TINY_C");
    if (value == (char *)0 || strcmp(value, "yes") != 0) {
        return 2;
    }

    io_path = getenv("MINI_IO_PATH");
    if (io_path == (char *)0 || *io_path == '\0') {
        return 3;
    }

    errno = EIO;
    if (strtol("123x", &end, 10) != 123 || *end != 'x' || errno != EIO) {
        return 4;
    }

    buffer = malloc(32);
    if (buffer == (char *)0) {
        return 5;
    }
    strcpy(buffer, "tiny-c-integration-ok");
    if (strlen(buffer) != 21 || strcmp(buffer, "tiny-c-integration-ok") != 0) {
        free(buffer);
        return 6;
    }

    stream = fopen(io_path, "w");
    if (stream == (FILE *)0 || fputs("AB", stream) == EOF ||
        fclose(stream) != 0) {
        free(buffer);
        return 7;
    }

    stream = fopen(io_path, "a");
    if (stream == (FILE *)0 || fputc('C', stream) != 'C' ||
        fclose(stream) != 0) {
        free(buffer);
        return 8;
    }

    stream = fopen(io_path, "r");
    if (stream == (FILE *)0) {
        free(buffer);
        return 9;
    }
    if (fgetc(stream) != 'A' || fgetc(stream) != 'B' ||
        fgetc(stream) != 'C' || fgetc(stream) != EOF ||
        !feof(stream) || ferror(stream)) {
        fclose(stream);
        free(buffer);
        return 10;
    }
    if (fclose(stream) != 0) {
        free(buffer);
        return 11;
    }

    if (stdout == (FILE *)0 || fputs(buffer, stdout) == EOF ||
        fputc('\n', stdout) == EOF || ferror(stdout)) {
        free(buffer);
        return 12;
    }
    free(buffer);
    return 0;
}
