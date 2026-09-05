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
    if (stream == (FILE *)0 || errno != ERANGE ||
        fputs("publish", stream) == EOF || fclose(stream) != 0) {
        if (stream != (FILE *)0 && ferror(stream)) {
            fclose(stream);
        }
        remove_if_present(argv[1]);
        return 3;
    }

    errno = ERANGE;
    stream = fopen(argv[1], "wx");
    if (stream != (FILE *)0 || errno != EEXIST) {
        if (stream != (FILE *)0) {
            fclose(stream);
        }
        remove_if_present(argv[1]);
        return 4;
    }

    errno = ERANGE;
    if (rename(argv[1], argv[2]) != 0 || errno != ERANGE) {
        remove_if_present(argv[1]);
        remove_if_present(argv[2]);
        return 5;
    }

    stream = fopen(argv[2], "r");
    if (stream == (FILE *)0 || fread(buffer, 1, 7, stream) != 7 ||
        fclose(stream) != 0) {
        if (stream != (FILE *)0 && ferror(stream)) {
            fclose(stream);
        }
        remove_if_present(argv[2]);
        return 6;
    }
    buffer[7] = '\0';
    if (strcmp(buffer, "publish") != 0) {
        remove_if_present(argv[2]);
        return 7;
    }

    errno = ERANGE;
    if (remove(argv[2]) != 0 || errno != ERANGE) {
        remove_if_present(argv[2]);
        return 8;
    }
    errno = ERANGE;
    if (remove(argv[2]) != -1 || errno != ENOENT) {
        return 9;
    }

    if (puts(ok) == EOF) {
        return 10;
    }
    return 0;
}
