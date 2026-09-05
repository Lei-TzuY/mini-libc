#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv, char **envp)
{
    static const char expected_output[] =
        "tiny-c-integration-ok:+00007:0x2a:-5000000000:11:22:33\n";
    char *end;
    char *value;
    char *io_path;
    char *buffer;
    char io_buffer[11];
    FILE *stream;
    int formatted;

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
        fgetc(stream) != '3' || ftell(stream) != 4L ||
        ungetc('!', stream) != '!' || ftell(stream) != 3L ||
        fgetc(stream) != '!' || ftell(stream) != 4L) {
        fclose(stream);
        free(buffer);
        return 11;
    }
    if (fgets(io_buffer, 5, stream) != io_buffer || strcmp(io_buffer, "45XY") != 0 ||
        ftell(stream) != 8L || fread(io_buffer, 1, 2, stream) != 2 ||
        io_buffer[0] != '8' || io_buffer[1] != '9' ||
        fgetc(stream) != EOF || !feof(stream)) {
        fclose(stream);
        free(buffer);
        return 12;
    }
    if (fclose(stream) != 0) {
        free(buffer);
        return 13;
    }

    formatted = printf("%s:%+06d:%#x:%lld:%u:%u:%u\n",
                       buffer, 7, 0x2aU, -5000000000LL, 11U, 22U, 33U);
    if (stdout == (FILE *)0 ||
        formatted != (int)(sizeof(expected_output) - 1U) || ferror(stdout)) {
        free(buffer);
        return 14;
    }

    free(buffer);
    return 0;
}
