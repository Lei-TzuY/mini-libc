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
    if (fgetc(stream) != EOF || fclose(stream) != 0) {
        return 0;
    }
    return 1;
}

int main(int argc, char **argv)
{
    static const char ok[] = "tiny-buffering-ok\n";
    char full[4];
    char line[8];
    char standard[BUFSIZ];
    char temporary[4];
    FILE *stream;

    if (argc != 2) {
        return 1;
    }

    stream = fopen(argv[1], "w+");
    if (stream == (FILE *)0 || setvbuf(stream, full, _IOFBF, sizeof(full)) != 0 ||
        fputs("abcd", stream) == EOF || !file_equals(argv[1], "", 0) ||
        fputc('e', stream) != 'e' || !file_equals(argv[1], "abcd", 4) ||
        fflush(stream) != 0 || !file_equals(argv[1], "abcde", 5)) {
        if (stream != (FILE *)0) {
            fclose(stream);
        }
        return 2;
    }

    if (setvbuf(stream, line, _IOLBF, sizeof(line)) != 0 ||
        fputc('F', stream) != 'F' || !file_equals(argv[1], "abcde", 5) ||
        fputc('\n', stream) != '\n' || !file_equals(argv[1], "abcdeF\n", 7)) {
        fclose(stream);
        return 3;
    }

    if (setvbuf(stream, (char *)0, _IONBF, 0U) != 0 ||
        fputc('G', stream) != 'G' || !file_equals(argv[1], "abcdeF\nG", 8)) {
        fclose(stream);
        return 4;
    }

    setbuf(stream, standard);
    if (fputc('H', stream) != 'H' || !file_equals(argv[1], "abcdeF\nG", 8) ||
        fflush(stream) != 0 || !file_equals(argv[1], "abcdeF\nGH", 9)) {
        fclose(stream);
        return 5;
    }

    if (setvbuf(stream, (char *)0, _IOFBF, 16U) != 0 ||
        fputc('I', stream) != 'I' || fclose(stream) != 0 ||
        !file_equals(argv[1], "abcdeF\nGHI", 10)) {
        return 6;
    }

    stream = tmpfile();
    if (stream == (FILE *)0 ||
        setvbuf(stream, temporary, _IOFBF, sizeof(temporary)) != 0 ||
        fprintf(stream, "tmp:%d", 7) != 5 || ftell(stream) != 5L) {
        if (stream != (FILE *)0) {
            fclose(stream);
        }
        return 7;
    }
    rewind(stream);
    if (fgetc(stream) != 't' || fgetc(stream) != 'm' ||
        fgetc(stream) != 'p' || fgetc(stream) != ':' ||
        fgetc(stream) != '7' || fgetc(stream) != EOF || !feof(stream) ||
        fclose(stream) != 0) {
        return 8;
    }

    if (fwrite(ok, 1, sizeof(ok) - 1U, stdout) != sizeof(ok) - 1U) {
        return 9;
    }
    return 0;
}
