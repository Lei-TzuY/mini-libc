#include <errno.h>
#include <stdio.h>
#include <string.h>

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
    static const char ok[] = "tiny-pathname-ok";
    FILE *stream;
    char buffer[8];

    if (argc != 3) {
        return 1;
    }
    if (!remove_if_present(argv[1]) || !remove_if_present(argv[2])) {
        return 2;
    }

    errno = ERANGE;
    stream = fopen(argv[1], "w+bx");
    if (stream == (FILE *)0 || errno != ERANGE) {
        remove_if_present(argv[1]);
        return 3;
    }
    if (fputs("publish", stream) == EOF || ferror(stream) || errno != ERANGE) {
        fclose(stream);
        remove_if_present(argv[1]);
        return 4;
    }
    if (fclose(stream) != 0 || errno != ERANGE) {
        remove_if_present(argv[1]);
        return 5;
    }

    errno = ERANGE;
    stream = fopen(argv[1], "wx");
    if (stream != (FILE *)0 || errno != EEXIST) {
        if (stream != (FILE *)0) {
            fclose(stream);
        }
        remove_if_present(argv[1]);
        return 6;
    }

    errno = ERANGE;
    if (rename(argv[1], argv[2]) != 0 || errno != ERANGE) {
        remove_if_present(argv[1]);
        remove_if_present(argv[2]);
        return 7;
    }

    stream = fopen(argv[2], "r");
    if (stream == (FILE *)0) {
        remove_if_present(argv[2]);
        return 8;
    }
    if (fread(buffer, 1, 7, stream) != 7 || ferror(stream)) {
        fclose(stream);
        remove_if_present(argv[2]);
        return 9;
    }
    if (fclose(stream) != 0) {
        remove_if_present(argv[2]);
        return 10;
    }
    buffer[7] = '\0';
    if (strcmp(buffer, "publish") != 0) {
        remove_if_present(argv[2]);
        return 11;
    }

    errno = ERANGE;
    if (remove(argv[2]) != 0 || errno != ERANGE) {
        remove_if_present(argv[2]);
        return 12;
    }
    errno = ERANGE;
    if (remove(argv[2]) != -1 || errno != ENOENT) {
        return 13;
    }

    if (puts(ok) == EOF) {
        return 14;
    }
    return 0;
}
