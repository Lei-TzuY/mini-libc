#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int tiny_vsnprintf(char *buffer, size_t size, const char *format, ...)
{
    va_list ap;
    int result;

    va_start(ap, format);
    result = vsnprintf(buffer, size, format, ap);
    va_end(ap);
    return result;
}

static int tiny_vfprintf(FILE *stream, const char *format, ...)
{
    va_list ap;
    int result;

    va_start(ap, format);
    result = vfprintf(stream, format, ap);
    va_end(ap);
    return result;
}

static int tiny_vprintf(const char *format, ...)
{
    va_list ap;
    int result;

    va_start(ap, format);
    result = vprintf(format, ap);
    va_end(ap);
    return result;
}

static int tiny_vfscanf(FILE *stream, const char *format, ...)
{
    va_list ap;
    int result;

    va_start(ap, format);
    result = vfscanf(stream, format, ap);
    va_end(ap);
    return result;
}

static int tiny_vscanf(const char *format, ...)
{
    va_list ap;
    int result;

    va_start(ap, format);
    result = vscanf(format, ap);
    va_end(ap);
    return result;
}

static int tiny_vsscanf(const char *source, const char *format, ...)
{
    va_list ap;
    int result;

    va_start(ap, format);
    result = vsscanf(source, format, ap);
    va_end(ap);
    return result;
}

static int tiny_va_sum(int count, ...)
{
    va_list ap;
    va_list copy;
    int first_copy;
    int total = 0;
    int i;

    va_start(ap, count);
    va_copy(copy, ap);
    first_copy = va_arg(copy, int);
    va_end(copy);
    for (i = 0; i < count; ++i) {
        total += va_arg(ap, int);
    }
    va_end(ap);
    return first_copy == 1 ? total : -1;
}

int main(int argc, char **argv, char **envp)
{
    static const char expected_output[] =
        "tiny-c-integration-ok:+00007:0x2a:-5000000000:11:22:33\n";
    static const char expected_memory_output[] = "mem:1:2:3:4:5";
    char *end;
    char *value;
    char *io_path;
    char *buffer;
    char io_buffer[11];
    char scan_digit[2];
    char scan_letters[3];
    char memory_letters[3];
    char format_buffer[32];
    char trunc_buffer[6];
    FILE *stream;
    int scan_auto;
    unsigned int scan_decimal;
    unsigned int scan_hex;
    int memory_auto;
    int memory_oct;
    unsigned int memory_hex;
    int memory_last;
    int stdin_values[6] = {0, 0, 0, 0, 0, 0};
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

    rewind(stream);
    if (tiny_vfscanf(stream, "%3i%2u%1[0-9]%2[A-Z]%2x",
                     &scan_auto, &scan_decimal, scan_digit, scan_letters,
                     &scan_hex) != 5 ||
        scan_auto != 10 || scan_decimal != 34U ||
        scan_digit[0] != '5' || scan_digit[1] != '\0' ||
        scan_letters[0] != 'X' || scan_letters[1] != 'Y' ||
        scan_letters[2] != '\0' || scan_hex != 0x89U || ftell(stream) != 10L) {
        fclose(stream);
        free(buffer);
        return 13;
    }
    if (fclose(stream) != 0) {
        free(buffer);
        return 14;
    }

    errno = EIO;
    if (tiny_vsscanf("0x2a 077 AB 89 7", "%i %i %2[A-Z] %x %d",
                     &memory_auto, &memory_oct, memory_letters, &memory_hex,
                     &memory_last) != 5 ||
        memory_auto != 42 || memory_oct != 63 ||
        memory_letters[0] != 'A' || memory_letters[1] != 'B' ||
        memory_letters[2] != '\0' || memory_hex != 0x89U ||
        memory_last != 7 || errno != EIO) {
        free(buffer);
        return 15;
    }

    if (tiny_va_sum(7, 1, 2, 3, 4, 5, 6, 7) != 28 || errno != EIO) {
        free(buffer);
        return 16;
    }

    formatted = tiny_vsnprintf(format_buffer, sizeof(format_buffer),
                               "mem:%d:%d:%d:%d:%d", 1, 2, 3, 4, 5);
    if (formatted != (int)(sizeof(expected_memory_output) - 1U) ||
        strcmp(format_buffer, expected_memory_output) != 0 || errno != EIO) {
        free(buffer);
        return 17;
    }

    formatted = snprintf(trunc_buffer, sizeof(trunc_buffer), "value:%u", 123U);
    if (formatted != 9 || strcmp(trunc_buffer, "value") != 0 || errno != EIO) {
        free(buffer);
        return 18;
    }

    stream = fopen(io_path, "w");
    if (stream == (FILE *)0) {
        free(buffer);
        return 19;
    }
    formatted = tiny_vfprintf(stream, "%c%c%c%c%c%c%c%c%x",
                              '0', '1', '2', '3', '4', '5', 'X', 'Y', 0x89U);
    if (formatted != 10 || errno != EIO || ferror(stream)) {
        fclose(stream);
        free(buffer);
        return 20;
    }
    if (fclose(stream) != 0) {
        free(buffer);
        return 20;
    }

    if (tiny_vscanf("%d %d %d %d %d %d",
                    &stdin_values[0], &stdin_values[1], &stdin_values[2],
                    &stdin_values[3], &stdin_values[4], &stdin_values[5]) != 6 ||
        stdin_values[0] != 1 || stdin_values[1] != 2 || stdin_values[2] != 3 ||
        stdin_values[3] != 4 || stdin_values[4] != 5 || stdin_values[5] != 6 ||
        errno != EIO) {
        free(buffer);
        return 21;
    }

    formatted = tiny_vprintf("%s:%+06d:%#x:%lld:%u:%u:%u\n",
                             buffer, 7, 0x2aU, -5000000000LL,
                             11U, 22U, 33U);
    if (stdout == (FILE *)0 ||
        formatted != (int)(sizeof(expected_output) - 1U) || ferror(stdout)) {
        free(buffer);
        return 22;
    }

    free(buffer);
    return 0;
}
