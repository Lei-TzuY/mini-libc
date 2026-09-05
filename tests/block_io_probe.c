#include <errno.h>
#include <mini/syscall.h>
#include <stddef.h>
#include <stdio.h>

static int same_bytes(const unsigned char *left, const char *right, size_t length)
{
    size_t i;

    for (i = 0; i < length; ++i) {
        if (left[i] != (unsigned char)right[i]) {
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    static const char path[] = "build/block_io_probe.tmp";
    static const char input_path[] = "build/input_buffer_probe.tmp";
    static const char ok[] = "block-io-ok\n";
    unsigned char buffer[8];
    unsigned char large[300];
    unsigned char middle[254];
    char line[16];
    FILE *stream;
    size_t count;
    size_t i;

    stream = fopen(path, "w+");
    if (stream == (FILE *)0) {
        return 1;
    }

    errno = ERANGE;
    if (fwrite("ABCDEFGHI", 3, 3, stream) != 3 || errno != ERANGE ||
        ferror(stream) || ftell(stream) != 9L) {
        fclose(stream);
        return 2;
    }

    if (fseek(stream, 0L, SEEK_SET) != 0 || ftell(stream) != 0L ||
        errno != ERANGE) {
        fclose(stream);
        return 3;
    }

    count = fread(buffer, 3, 2, stream);
    if (count != 2 || !same_bytes(buffer, "ABCDEF", 6) ||
        ftell(stream) != 6L || feof(stream) || ferror(stream)) {
        fclose(stream);
        return 4;
    }

    if (fseek(stream, -3L, SEEK_CUR) != 0 || ftell(stream) != 3L) {
        fclose(stream);
        return 5;
    }
    if (fread(buffer, 1, 6, stream) != 6 ||
        !same_bytes(buffer, "DEFGHI", 6) || feof(stream)) {
        fclose(stream);
        return 6;
    }

    if (fread(buffer, 1, 1, stream) != 0 || !feof(stream) || ferror(stream)) {
        fclose(stream);
        return 7;
    }
    if (fseek(stream, 0L, SEEK_SET) != 0 || feof(stream) ||
        fread(buffer, 1, 1, stream) != 1 || buffer[0] != 'A') {
        fclose(stream);
        return 8;
    }

    if (fseek(stream, -1L, SEEK_END) != 0 || ftell(stream) != 8L ||
        fread(buffer, 1, 1, stream) != 1 || buffer[0] != 'I') {
        fclose(stream);
        return 9;
    }

    rewind(stream);
    if (ftell(stream) != 0L || feof(stream) || ferror(stream)) {
        fclose(stream);
        return 10;
    }
    if (fclose(stream) != 0) {
        return 11;
    }

    stream = fopen(path, "w+");
    if (stream == (FILE *)0) {
        return 12;
    }
    if (fwrite("ABCDE", 1, 5, stream) != 5 ||
        fseek(stream, 0L, SEEK_SET) != 0) {
        fclose(stream);
        return 13;
    }

    buffer[5] = 0x7fU;
    count = fread(buffer, 2, 3, stream);
    if (count != 2 || !same_bytes(buffer, "ABCDE", 5) ||
        buffer[5] != 0x7fU || !feof(stream) || ferror(stream)) {
        fclose(stream);
        return 14;
    }

    errno = ERANGE;
    if (fread(buffer, 0, 99, stream) != 0 ||
        fwrite(buffer, 99, 0, stream) != 0 || errno != ERANGE) {
        fclose(stream);
        return 15;
    }

    if (fclose(stream) != 0) {
        return 16;
    }

    for (i = 0; i < sizeof(large); ++i) {
        large[i] = (unsigned char)'R';
    }
    large[0] = (unsigned char)'A';
    for (i = 1; i < 255; ++i) {
        large[i] = (unsigned char)'q';
    }
    large[255] = (unsigned char)'L';
    large[256] = (unsigned char)'I';
    large[257] = (unsigned char)'N';
    large[258] = (unsigned char)'E';
    large[259] = (unsigned char)'\n';

    stream = fopen(input_path, "w+");
    if (stream == (FILE *)0 || fwrite(large, 1, sizeof(large), stream) != sizeof(large) ||
        fseek(stream, 0L, SEEK_SET) != 0) {
        if (stream != (FILE *)0) {
            fclose(stream);
        }
        return 17;
    }

    if (fgetc(stream) != 'A' || ftell(stream) != 1L || feof(stream) ||
        ferror(stream)) {
        fclose(stream);
        return 18;
    }
    if (ungetc('Z', stream) != 'Z' || ftell(stream) != 0L || feof(stream) ||
        fgetc(stream) != 'Z' || ftell(stream) != 1L) {
        fclose(stream);
        return 19;
    }

    if (fread(middle, 1, sizeof(middle), stream) != sizeof(middle) ||
        ftell(stream) != 255L) {
        fclose(stream);
        return 20;
    }
    for (i = 0; i < sizeof(middle); ++i) {
        if (middle[i] != (unsigned char)'q') {
            fclose(stream);
            return 21;
        }
    }

    if (fgets(line, (int)sizeof(line), stream) != line ||
        line[0] != 'L' || line[1] != 'I' || line[2] != 'N' ||
        line[3] != 'E' || line[4] != '\n' || line[5] != '\0' ||
        ftell(stream) != 260L) {
        fclose(stream);
        return 22;
    }

    if (ungetc('!', stream) != '!' || ftell(stream) != 259L ||
        fgetc(stream) != '!' || ftell(stream) != 260L) {
        fclose(stream);
        return 23;
    }

    if (fread(large, 1, 40, stream) != 40) {
        fclose(stream);
        return 24;
    }
    for (i = 0; i < 40; ++i) {
        if (large[i] != (unsigned char)'R') {
            fclose(stream);
            return 25;
        }
    }
    if (fgetc(stream) != EOF || !feof(stream) || ferror(stream)) {
        fclose(stream);
        return 26;
    }

    errno = ERANGE;
    if (ungetc(EOF, stream) != EOF || errno != ERANGE || !feof(stream)) {
        fclose(stream);
        return 27;
    }
    if (fputc('X', stream) != 'X' || feof(stream) == 0 || ftell(stream) != 301L) {
        fclose(stream);
        return 28;
    }
    if (fclose(stream) != 0) {
        return 29;
    }

    if (mini_sys_write(1, ok, sizeof(ok) - 1) != (long)(sizeof(ok) - 1)) {
        return 30;
    }
    return 0;
}
