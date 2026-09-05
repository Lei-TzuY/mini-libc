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

static int remove_if_present(const char *path)
{
    errno = ERANGE;
    if (remove(path) == 0) {
        return 1;
    }
    return errno == ENOENT;
}

int main(int argc, char **argv)
{
    static const char ok[] = "file-stream-ok\n";
    static const char exclusive_path[] = "build/file-stream-exclusive.tmp";
    static const char renamed_path[] = "build/file-stream-renamed.tmp";
    char full_buffer[2];
    char line_buffer[8];
    char setbuf_storage[BUFSIZ];
    char tmp_buffer[4];
    char tmp_word[4];
    FILE *stream;
    int tmp_value;

    if (argc != 2) {
        return 1;
    }

    if (!remove_if_present(exclusive_path) || !remove_if_present(renamed_path)) {
        return 2;
    }

    errno = ERANGE;
    stream = fopen(argv[1], "r");
    if (stream != (FILE *)0 || errno != ENOENT) {
        if (stream != (FILE *)0) {
            fclose(stream);
        }
        return 3;
    }

    errno = ERANGE;
    stream = fopen(argv[1], "w");
    if (stream == (FILE *)0 || errno != ERANGE) {
        return 4;
    }
    if (fputs("OLD", stream) == EOF || ferror(stream) || errno != ERANGE) {
        fclose(stream);
        return 5;
    }
    if (fclose(stream) != 0 || errno != ERANGE) {
        return 6;
    }

    stream = fopen(argv[1], "wb");
    if (stream == (FILE *)0 ||
        setvbuf(stream, full_buffer, _IOFBF, sizeof(full_buffer)) != 0) {
        return 7;
    }
    if (fputc('A', stream) != 'A' || full_buffer[0] != 'A' ||
        fputc('B', stream) != 'B' || !file_equals(argv[1], "", 0)) {
        fclose(stream);
        return 8;
    }
    if (fputc('C', stream) != 'C' || !file_equals(argv[1], "AB", 2) ||
        fflush(stream) != 0 || !file_equals(argv[1], "ABC", 3) ||
        fclose(stream) != 0) {
        return 9;
    }

    stream = fopen(argv[1], "a");
    if (stream == (FILE *)0 ||
        setvbuf(stream, line_buffer, _IOLBF, sizeof(line_buffer)) != 0) {
        return 10;
    }
    if (fputc('D', stream) != 'D' || !file_equals(argv[1], "ABC", 3) ||
        fputc('\n', stream) != '\n' || !file_equals(argv[1], "ABCD\n", 5) ||
        fclose(stream) != 0) {
        return 11;
    }

    stream = fopen(argv[1], "wb");
    if (stream == (FILE *)0) {
        return 12;
    }
    setbuf(stream, setbuf_storage);
    if (fputs("AB", stream) == EOF || !file_equals(argv[1], "", 0) ||
        fclose(stream) != 0 || !file_equals(argv[1], "AB", 2)) {
        return 13;
    }

    stream = fopen(argv[1], "a");
    if (stream == (FILE *)0 || setvbuf(stream, (char *)0, _IOFBF, 4U) != 0 ||
        setvbuf(stream, (char *)0, _IONBF, 0U) != 0 ||
        fputc('C', stream) != 'C' || !file_equals(argv[1], "ABC", 3) ||
        fclose(stream) != 0) {
        return 14;
    }

    stream = fopen(argv[1], "r");
    if (stream == (FILE *)0) {
        return 15;
    }
    if (fgetc(stream) != 'A' || fgetc(stream) != 'B' ||
        fgetc(stream) != 'C' || fgetc(stream) != EOF ||
        !feof(stream) || ferror(stream)) {
        fclose(stream);
        return 16;
    }
    if (fclose(stream) != 0) {
        return 17;
    }

    errno = ERANGE;
    if (fopen(argv[1], "r++") != (FILE *)0 || errno != EINVAL) {
        return 18;
    }
    errno = ERANGE;
    if (fopen(argv[1], "rx") != (FILE *)0 || errno != EINVAL) {
        return 19;
    }
    errno = ERANGE;
    if (fopen(argv[1], "wxx") != (FILE *)0 || errno != EINVAL) {
        return 20;
    }

    errno = ERANGE;
    stream = fopen(exclusive_path, "w+bx");
    if (stream == (FILE *)0 || errno != ERANGE) {
        remove_if_present(exclusive_path);
        return 21;
    }
    if (fputs("atomic", stream) == EOF || ferror(stream) || errno != ERANGE) {
        fclose(stream);
        remove_if_present(exclusive_path);
        return 22;
    }
    if (fclose(stream) != 0 || errno != ERANGE ||
        !file_equals(exclusive_path, "atomic", 6)) {
        remove_if_present(exclusive_path);
        return 23;
    }

    errno = ERANGE;
    stream = fopen(exclusive_path, "wx");
    if (stream != (FILE *)0 || errno != EEXIST ||
        !file_equals(exclusive_path, "atomic", 6)) {
        if (stream != (FILE *)0) {
            fclose(stream);
        }
        remove_if_present(exclusive_path);
        return 24;
    }

    errno = ERANGE;
    if (rename(exclusive_path, renamed_path) != 0 || errno != ERANGE ||
        !file_equals(renamed_path, "atomic", 6)) {
        remove_if_present(exclusive_path);
        remove_if_present(renamed_path);
        return 25;
    }
    errno = ERANGE;
    stream = fopen(exclusive_path, "r");
    if (stream != (FILE *)0 || errno != ENOENT) {
        if (stream != (FILE *)0) {
            fclose(stream);
        }
        remove_if_present(renamed_path);
        return 26;
    }
    errno = ERANGE;
    if (remove(renamed_path) != 0 || errno != ERANGE) {
        remove_if_present(renamed_path);
        return 27;
    }
    errno = ERANGE;
    if (remove(renamed_path) != -1 || errno != ENOENT) {
        return 28;
    }
    errno = ERANGE;
    if (rename(exclusive_path, renamed_path) != -1 || errno != ENOENT) {
        return 29;
    }

    errno = ERANGE;
    stream = tmpfile();
    if (stream == (FILE *)0 || errno != ERANGE ||
        setvbuf(stream, tmp_buffer, _IOFBF, sizeof(tmp_buffer)) != 0 ||
        fprintf(stream, "tmp:%d", 7) != 5 || ftell(stream) != 5L) {
        if (stream != (FILE *)0) {
            fclose(stream);
        }
        return 30;
    }
    rewind(stream);
    tmp_word[0] = '\0';
    tmp_value = 0;
    if (ferror(stream) || feof(stream) || ftell(stream) != 0L ||
        fscanf(stream, "%3[a-z]:%d", tmp_word, &tmp_value) != 2 ||
        tmp_word[0] != 't' || tmp_word[1] != 'm' || tmp_word[2] != 'p' ||
        tmp_word[3] != '\0' || tmp_value != 7 || ftell(stream) != 5L ||
        fseek(stream, 0L, SEEK_END) != 0 || fputc('!', stream) != '!' ||
        ftell(stream) != 6L) {
        fclose(stream);
        return 31;
    }
    rewind(stream);
    if (fgetc(stream) != 't' || fgetc(stream) != 'm' ||
        fgetc(stream) != 'p' || fgetc(stream) != ':' ||
        fgetc(stream) != '7' || fgetc(stream) != '!' ||
        fgetc(stream) != EOF || !feof(stream) || ferror(stream) ||
        fclose(stream) != 0) {
        return 32;
    }

    if (mini_sys_write(1, ok, sizeof(ok) - 1) != (long)(sizeof(ok) - 1)) {
        return 33;
    }
    return 0;
}
