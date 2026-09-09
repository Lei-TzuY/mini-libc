#include <errno.h>
#include <mini/syscall.h>
#include <stdio.h>
#include <wchar.h>

static int fail(FILE *stream, int code)
{
    if (stream != (FILE *)0) {
        (void)fclose(stream);
    }
    (void)remove("build/wide-stdio-probe.tmp");
    return code;
}

int main(void)
{
    static const char path[] = "build/wide-stdio-probe.tmp";
    static const char marker[] = "wide-stdio-ok\n";
    static const wchar_t tail[] = {'B', 'C', '\n', 0};
    static const wchar_t first_line[] = {'A', '\n', 0};
    static const wchar_t bounded[] = {'B', 'C', 0};
    static const wchar_t newline_only[] = {'\n', 0};
    static const wchar_t invalid_wide[] = {'O', (wchar_t)0x80, 0};
    static const wchar_t stdin_tail[] = {'O', 'W', '\n', 0};
    static const wchar_t stdout_tail[] = {'O', 'K', 0};
    wchar_t line[8];
    FILE *stream;

    (void)remove(path);
    stream = fopen(path, "w+");
    if (stream == (FILE *)0) {
        return 1;
    }

    errno = ERANGE;
    if (fwide(stream, 0) != 0 || errno != ERANGE ||
        fwide(stream, 1) <= 0 || fwide(stream, -1) <= 0 || errno != ERANGE) {
        return fail(stream, 2);
    }
    if (fputwc((wchar_t)'A', stream) != (wint_t)'A' ||
        putwc((wchar_t)'\n', stream) != (wint_t)'\n' ||
        fputws(tail, stream) < 0 || ftell(stream) != 5L) {
        return fail(stream, 3);
    }

    errno = ERANGE;
    if (fputc('x', stream) != EOF || errno != EINVAL || !ferror(stream)) {
        return fail(stream, 4);
    }
    clearerr(stream);
    errno = ERANGE;
    if (fprintf(stream, "x") != EOF || errno != EINVAL || !ferror(stream)) {
        return fail(stream, 5);
    }
    clearerr(stream);
    errno = ERANGE;
    if (fwrite("x", 1U, 1U, stream) != 0U || errno != EINVAL ||
        !ferror(stream)) {
        return fail(stream, 6);
    }
    clearerr(stream);

    if (fseek(stream, 0L, SEEK_SET) != 0 || fwide(stream, 0) <= 0 ||
        fgetws(line, 8, stream) != line || wcscmp(line, first_line) != 0 ||
        ftell(stream) != 2L || fgetws(line, 3, stream) != line ||
        wcscmp(line, bounded) != 0 || ftell(stream) != 4L ||
        ungetwc((wint_t)'Z', stream) != (wint_t)'Z' || ftell(stream) != 3L ||
        getwc(stream) != (wint_t)'Z' || ftell(stream) != 4L ||
        fgetws(line, 8, stream) != line || wcscmp(line, newline_only) != 0 ||
        ftell(stream) != 5L || fgetws(line, 8, stream) != (wchar_t *)0 ||
        !feof(stream)) {
        return fail(stream, 7);
    }

    rewind(stream);
    line[0] = (wchar_t)'?';
    if (fwide(stream, 0) <= 0 || feof(stream) || ferror(stream) ||
        ftell(stream) != 0L || fgetws(line, 1, stream) != line || line[0] != 0 ||
        ftell(stream) != 0L) {
        return fail(stream, 8);
    }

    stream = freopen(path, "w+", stream);
    if (stream == (FILE *)0 || fwide(stream, 0) != 0 ||
        fputc('B', stream) != 'B' || fwide(stream, 1) >= 0) {
        return fail(stream, 9);
    }
    errno = ERANGE;
    if (fputws(tail, stream) != EOF || errno != EINVAL || !ferror(stream)) {
        return fail(stream, 10);
    }
    clearerr(stream);
    if (fclose(stream) != 0) {
        return fail((FILE *)0, 11);
    }

    stream = tmpfile();
    if (stream == (FILE *)0 || fwide(stream, 1) <= 0) {
        return fail(stream, 12);
    }
    errno = ERANGE;
    if (fputws(invalid_wide, stream) != EOF || errno != EILSEQ ||
        !ferror(stream)) {
        return fail(stream, 13);
    }
    clearerr(stream);
    if (fclose(stream) != 0) {
        return fail((FILE *)0, 14);
    }

    stream = fopen(path, "w");
    if (stream == (FILE *)0) {
        return fail((FILE *)0, 15);
    }
    if (fputc(0x80, stream) != 0x80) {
        return fail(stream, 15);
    }
    if (fclose(stream) != 0) {
        return fail((FILE *)0, 15);
    }

    stream = fopen(path, "r");
    if (stream == (FILE *)0 || fwide(stream, 1) <= 0) {
        return fail(stream, 16);
    }
    errno = ERANGE;
    if (fgetws(line, 8, stream) != (wchar_t *)0 || errno != EILSEQ ||
        !ferror(stream)) {
        return fail(stream, 17);
    }
    clearerr(stream);
    if (fclose(stream) != 0) {
        return fail((FILE *)0, 18);
    }

    errno = ERANGE;
    if (fwide(stdin, 0) != 0 || getwchar() != (wint_t)'R' ||
        fgetws(line, 8, stdin) != line || wcscmp(line, stdin_tail) != 0 ||
        fwide(stdin, 0) <= 0 || errno != ERANGE) {
        return fail((FILE *)0, 19);
    }
    if (fwide(stdout, 0) != 0 || putwchar((wchar_t)'!') != (wint_t)'!' ||
        fputws(stdout_tail, stdout) < 0 || fwide(stdout, 0) <= 0 ||
        fflush(stdout) != 0) {
        return fail((FILE *)0, 20);
    }

    (void)remove(path);
    if (mini_sys_write(1, marker, sizeof(marker) - 1U) !=
        (long)(sizeof(marker) - 1U)) {
        return 21;
    }
    return 0;
}
