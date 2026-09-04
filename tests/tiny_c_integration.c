#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv, char **envp)
{
    char *end;
    char *value;
    char *buffer;

    (void)envp;

    if (argc != 2 || strcmp(argv[1], "arg") != 0) {
        return 1;
    }

    value = getenv("MINI_TINY_C");
    if (value == (char *)0 || strcmp(value, "yes") != 0) {
        return 2;
    }

    errno = EIO;
    if (strtol("123x", &end, 10) != 123 || *end != 'x' || errno != EIO) {
        return 3;
    }

    buffer = malloc(32);
    if (buffer == (char *)0) {
        return 4;
    }
    strcpy(buffer, "tiny-c-integration-ok");
    if (strlen(buffer) != 21 || strcmp(buffer, "tiny-c-integration-ok") != 0) {
        free(buffer);
        return 5;
    }

    if (stdout == (FILE *)0 || fputs(buffer, stdout) == EOF ||
        fputc('\n', stdout) == EOF || ferror(stdout)) {
        free(buffer);
        return 6;
    }
    free(buffer);
    return 0;
}
