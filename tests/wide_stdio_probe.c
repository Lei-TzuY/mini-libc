#include <errno.h>
#include <mini/syscall.h>
#include <stdarg.h>
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

static int call_vfwprintf(FILE *stream, const wchar_t *format, ...)
{
    va_list ap;
    int result;

    va_start(ap, format);
    result = vfwprintf(stream, format, ap);
    va_end(ap);
    return result;
}

static int call_vswprintf(wchar_t *buffer, size_t size,
                          const wchar_t *format, ...)
{
    va_list ap;
    int result;

    va_start(ap, format);
    result = vswprintf(buffer, size, format, ap);
    va_end(ap);
    return result;
}

static int call_vwprintf(const wchar_t *format, ...)
{
    va_list ap;
    int result;

    va_start(ap, format);
    result = vwprintf(format, ap);
    va_end(ap);
    return result;
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
    static const wchar_t memory_expected[] = {'O', 'K', ':', '7', ':', '2', '.', '5', 0};
    static const wchar_t ordinary_expected[] = {'0', 'x', '2', 'a', '/', '9', 0};
    static const wchar_t stream_expected[] = {'N', '=', '7', ' ', 'S', '=', 'o', 'k', ' ', 'F', '=', '2', '.', '5', '\n', 0};
    static const wchar_t truncated_expected[] = {'a', 'b', 'c', 'd', 0};
    static const wchar_t invalid_format[] = {(wchar_t)0x80, 0};
    static const char invalid_narrow[] = {(char)0x80, 0};
    wchar_t line[32];
    wchar_t formatted[64];
    wchar_t small[5];
    FILE *stream;
    int count;

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
    count = call_vswprintf(formatted, 64U, L"%s:%d:%.1f", "OK", 7, 2.5);
    if (count != (int)wcslen(memory_expected) ||
        wcscmp(formatted, memory_expected) != 0 || errno != ERANGE) {
        return fail((FILE *)0, 19);
    }
    count = swprintf(formatted, 64U, L"%#x/%u", 42U, 9U);
    if (count != (int)wcslen(ordinary_expected) ||
        wcscmp(formatted, ordinary_expected) != 0) {
        return fail((FILE *)0, 20);
    }
    errno = 0;
    if (swprintf(small, 5U, L"abcdef") != EOF || errno != ERANGE ||
        wcscmp(small, truncated_expected) != 0) {
        return fail((FILE *)0, 21);
    }
    errno = 0;
    if (swprintf((wchar_t *)0, 0U, L"") != EOF || errno != ERANGE) {
        return fail((FILE *)0, 22);
    }
    errno = 0;
    if (swprintf(formatted, 64U, invalid_format) != EOF || errno != EILSEQ) {
        return fail((FILE *)0, 23);
    }

    stream = tmpfile();
    if (stream == (FILE *)0 || fwide(stream, 1) <= 0 ||
        fwprintf(stream, L"N=%d S=%s ", 7, "ok") != 9 ||
        call_vfwprintf(stream, L"F=%.1f\n", 2.5) != 6 ||
        ftell(stream) != 15L) {
        return fail(stream, 24);
    }
    rewind(stream);
    if (fgetws(line, 32, stream) != line ||
        wcscmp(line, stream_expected) != 0 || fclose(stream) != 0) {
        return fail((FILE *)0, 25);
    }

    stream = tmpfile();
    if (stream == (FILE *)0 || fputc('B', stream) != 'B') {
        return fail(stream, 26);
    }
    errno = 0;
    if (fwprintf(stream, L"%d", 1) != EOF || errno != EINVAL ||
        !ferror(stream)) {
        return fail(stream, 27);
    }
    clearerr(stream);
    if (fclose(stream) != 0) {
        return fail((FILE *)0, 28);
    }

    stream = tmpfile();
    if (stream == (FILE *)0 || fwide(stream, 1) <= 0) {
        return fail(stream, 29);
    }
    errno = 0;
    if (fwprintf(stream, L"%s", invalid_narrow) != EOF || errno != EILSEQ ||
        !ferror(stream)) {
        return fail(stream, 30);
    }
    clearerr(stream);
    if (fclose(stream) != 0) {
        return fail((FILE *)0, 31);
    }

    errno = ERANGE;
    if (fwide(stdin, 0) != 0 || getwchar() != (wint_t)'R' ||
        fgetws(line, 8, stdin) != line || wcscmp(line, stdin_tail) != 0 ||
        fwide(stdin, 0) <= 0 || errno != ERANGE) {
        return fail((FILE *)0, 32);
    }
    if (fwide(stdout, 0) != 0 || putwchar((wchar_t)'!') != (wint_t)'!' ||
        fputws(stdout_tail, stdout) < 0 || wprintf(L":%d", 7) != 2 ||
        call_vwprintf(L":%.1f", 2.5) != 4 || fwide(stdout, 0) <= 0 ||
        fflush(stdout) != 0) {
        return fail((FILE *)0, 33);
    }

    (void)remove(path);
    if (mini_sys_write(1, marker, sizeof(marker) - 1U) !=
        (long)(sizeof(marker) - 1U)) {
        return 34;
    }
    return 0;
}
