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
    FILE *stream;
    wint_t wc;

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
        putwc((wchar_t)'\n', stream) != (wint_t)'\n' || ftell(stream) != 2L) {
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
        fgetwc(stream) != (wint_t)'A' || ftell(stream) != 1L ||
        ungetwc((wint_t)'Z', stream) != (wint_t)'Z' || ftell(stream) != 0L ||
        getwc(stream) != (wint_t)'Z' || ftell(stream) != 1L ||
        fgetwc(stream) != (wint_t)'\n' || ftell(stream) != 2L ||
        fgetwc(stream) != WEOF || !feof(stream)) {
        return fail(stream, 7);
    }

    rewind(stream);
    if (fwide(stream, 0) <= 0 || feof(stream) || ferror(stream) ||
        ftell(stream) != 0L) {
        return fail(stream, 8);
    }

    stream = freopen(path, "w+", stream);
    if (stream == (FILE *)0 || fwide(stream, 0) != 0 ||
        fputc('B', stream) != 'B' || fwide(stream, 1) >= 0) {
        return fail(stream, 9);
    }
    errno = ERANGE;
    if (fputwc((wchar_t)'C', stream) != WEOF || errno != EINVAL ||
        !ferror(stream)) {
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
    if (fputwc((wchar_t)0x80, stream) != WEOF || errno != EILSEQ ||
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
    wc = fgetwc(stream);
    if (wc != WEOF || errno != EILSEQ || !ferror(stream)) {
        return fail(stream, 17);
    }
    clearerr(stream);
    if (fclose(stream) != 0) {
        return fail((FILE *)0, 18);
    }

    errno = ERANGE;
    if (fwide(stdin, 0) != 0 || getwchar() != (wint_t)'R' ||
        fwide(stdin, 0) <= 0 || errno != ERANGE) {
        return fail((FILE *)0, 19);
    }
    if (fwide(stdout, 0) != 0 || putwchar((wchar_t)'!') != (wint_t)'!' ||
        fwide(stdout, 0) <= 0 || fflush(stdout) != 0) {
        return fail((FILE *)0, 20);
    }

    (void)remove(path);
    if (mini_sys_write(1, marker, sizeof(marker) - 1U) !=
        (long)(sizeof(marker) - 1U)) {
        return 21;
    }
    return 0;
}
