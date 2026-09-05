#include <errno.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    char buffer[4];
    FILE *stream;
    FILE *identity;

    if (argc != 3) {
        return 1;
    }

    stream = fopen(argv[1], "w+");
    if (stream == (FILE *)0 ||
        setvbuf(stream, buffer, _IOFBF, sizeof(buffer)) != 0 ||
        fputs("OLD", stream) == EOF) {
        if (stream != (FILE *)0) {
            fclose(stream);
        }
        return 2;
    }

    identity = stream;
    errno = ERANGE;
    if (freopen(argv[2], "w+", stream) != identity || errno != ERANGE ||
        fputs("NEW", stream) == EOF || fflush(stream) != 0) {
        return 3;
    }
    rewind(stream);
    if (fgetc(stream) != 'N' || fgetc(stream) != 'E' || fgetc(stream) != 'W' ||
        fgetc(stream) != EOF || !feof(stream) || ferror(stream) ||
        fclose(stream) != 0) {
        return 4;
    }

    stream = fopen(argv[1], "r");
    if (stream == (FILE *)0 || fgetc(stream) != 'O') {
        if (stream != (FILE *)0) {
            fclose(stream);
        }
        return 5;
    }
    errno = ERANGE;
    if (freopen(argv[2], "r++", stream) != (FILE *)0 || errno != EINVAL ||
        fgetc(stream) != 'L' || fclose(stream) != 0) {
        return 6;
    }

    if (puts("tiny-rebind-ok") == EOF) {
        return 7;
    }
    return 0;
}
