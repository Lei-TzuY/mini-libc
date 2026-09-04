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
    char io_buffer[11];
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

    stream = fopen(io_path, "w+");
    if (stream == (FILE *)0 || fwrite("0123456789", 2, 5, stream) != 5 ||
        ftell(stream) != 10L) {
        if (stream != (FILE *)0) {
            fclose(stream);
        }
        free(buffer);
        return 7;
    }

    if (fseek(stream, -4L, SEEK_END) != 0 || ftell(stream) != 6L ||
        fwrite("XY", 1, 2, stream) != 2 || ftell(stream) != 8L) {
        fclose(stream);
        free(buffer);
        return 8;
    }

    rewind(stream);
    if (ferror(stream) || feof(stream) || ftell(stream) != 0L ||
        fread(io_buffer, 2, 5, stream) != 5) {
        fclose(stream);
        free(buffer);
        return 9;
    }
    io_buffer[10] = '\0';
    if (strcmp(io_buffer, "012345XY89") != 0 ||
        fread(io_buffer, 1, 1, stream) != 0 || !feof(stream)) {
        fclose(stream);
        free(buffer);
        return 10;
    }

    if (fseek(stream, 3L, SEEK_SET) != 0 || feof(stream) ||
        fread(io_buffer, 1, 4, stream) != 4) {
        fclose(stream);
        free(buffer);
        return 11;
    }
    io_buffer[4] = '\0';
    if (strcmp(io_buffer, "345X") != 0 || fclose(stream) != 0) {
        free(buffer);
        return 12;
    }

    if (stdout == (FILE *)0 || fputs(buffer, stdout) == EOF ||
        fputc('\n', stdout) == EOF || ferror(stdout)) {
        free(buffer);
        return 13;
    }
    free(buffer);
    return 0;
}
