#include <errno.h>
#include <locale.h>
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

static int call_vfwscanf(FILE *stream, const wchar_t *format, ...)
{
    va_list ap;
    int result;

    va_start(ap, format);
    result = vfwscanf(stream, format, ap);
    va_end(ap);
    return result;
}

static int call_vswscanf(const wchar_t *input, const wchar_t *format, ...)
{
    va_list ap;
    int result;

    va_start(ap, format);
    result = vswscanf(input, format, ap);
    va_end(ap);
    return result;
}

static int call_vwscanf(const wchar_t *format, ...)
{
    va_list ap;
    int result;

    va_start(ap, format);
    result = vwscanf(format, ap);
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
    static const wchar_t narrow_wide[] = {'O', 'K', 0};
    static const wchar_t wide_arg_text[] = {'W', 'I', 'D', 'E', 0};
    static const wchar_t wide_arg_memory_expected[] = {'[', ' ', ' ', 'W', 'I', 'D', ']', '[', 'Q', ' ', ' ', ']', 0};
    static const wchar_t wide_arg_stream_value[] = {'W', 'X', 0};
    static const wchar_t wide_arg_stream_expected[] = {'[', 'W', 'X', ':', 'Q', ']', 0};
    static const wchar_t scan_memory[] = {'4', '2', ' ', 'O', 'K', ' ', '2', '.', '5', ' ', 'W', 'X', 0};
    static const wchar_t scan_memory_v[] = {'0', 'x', '2', 'a', ' ', 'A', 'B', 'C', ' ', 'Q', 0};
    static const wchar_t scan_file_first[] = {'1', '7', ' ', 'W', 'X', ' ', '3', '.', '5', ' ', 'Q', '\n', 0};
    static const wchar_t scan_file_second[] = {'2', '3', ' ', 'Y', 'Z', ' ', '4', '.', '5', ' ', 'R', '\n', 0};
    wchar_t line[32];
    wchar_t formatted[64];
    wchar_t small[5];
    wchar_t scan_wide[8];
    wchar_t scan_set[8];
    wchar_t scan_char = 0;
    char narrow[32];
    char scan_narrow[8];
    double scan_double = 0.0;
    int scan_integer = 0;
    int scan_second = 0;
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
    count = snprintf(narrow, sizeof(narrow), "[%ls:%lc]", narrow_wide,
                     (wint_t)'Q');
    if (count != 6 || narrow[0] != '[' || narrow[1] != 'O' ||
        narrow[2] != 'K' || narrow[3] != ':' || narrow[4] != 'Q' ||
        narrow[5] != ']' || narrow[6] != '\0' || errno != ERANGE) {
        return fail((FILE *)0, 35);
    }
    errno = ERANGE;
    count = swprintf(formatted, 64U, L"[%5.3ls][%-3lc]", wide_arg_text,
                     (wint_t)'Q');
    if (count != (int)wcslen(wide_arg_memory_expected) ||
        wcscmp(formatted, wide_arg_memory_expected) != 0 || errno != ERANGE) {
        return fail((FILE *)0, 36);
    }
    errno = 0;
    if (snprintf(narrow, sizeof(narrow), "%ls", invalid_wide) != EOF ||
        errno != EILSEQ) {
        return fail((FILE *)0, 37);
    }
    errno = 0;
    if (swprintf(formatted, 64U, L"%ls", invalid_wide) != EOF ||
        errno != EILSEQ) {
        return fail((FILE *)0, 38);
    }

    stream = tmpfile();
    if (stream == (FILE *)0 || fwide(stream, 1) <= 0 ||
        fwprintf(stream, L"[%ls:%lc]", wide_arg_stream_value,
                 (wint_t)'Q') != 6 || ftell(stream) != 6L) {
        return fail(stream, 39);
    }
    rewind(stream);
    if (fgetws(line, 32, stream) != line ||
        wcscmp(line, wide_arg_stream_expected) != 0 || fclose(stream) != 0) {
        return fail((FILE *)0, 40);
    }

    stream = tmpfile();
    if (stream == (FILE *)0 || fwide(stream, 1) <= 0) {
        return fail(stream, 41);
    }
    errno = 0;
    if (fwprintf(stream, L"%ls", invalid_wide) != EOF || errno != EILSEQ ||
        !ferror(stream)) {
        return fail(stream, 42);
    }
    clearerr(stream);
    if (fclose(stream) != 0) {
        return fail((FILE *)0, 43);
    }

    scan_integer = 0;
    scan_double = 0.0;
    scan_narrow[0] = '\0';
    scan_wide[0] = 0;
    errno = ERANGE;
    if (swscanf(scan_memory, L"%d %2s %lf %2ls", &scan_integer,
                scan_narrow, &scan_double, scan_wide) != 4 ||
        scan_integer != 42 || scan_narrow[0] != 'O' || scan_narrow[1] != 'K' ||
        scan_narrow[2] != '\0' || scan_double != 2.5 ||
        scan_wide[0] != (wchar_t)'W' || scan_wide[1] != (wchar_t)'X' ||
        scan_wide[2] != 0 || errno != ERANGE) {
        return fail((FILE *)0, 44);
    }

    scan_integer = 0;
    scan_set[0] = 0;
    scan_char = 0;
    if (call_vswscanf(scan_memory_v, L"%i %3l[A-Z] %lc", &scan_integer,
                      scan_set, &scan_char) != 3 || scan_integer != 42 ||
        scan_set[0] != (wchar_t)'A' || scan_set[1] != (wchar_t)'B' ||
        scan_set[2] != (wchar_t)'C' || scan_set[3] != 0 ||
        scan_char != (wchar_t)'Q') {
        return fail((FILE *)0, 45);
    }

    scan_wide[0] = 0;
    scan_char = 0;
    if (sscanf("OK Q", "%2ls %lc", scan_wide, &scan_char) != 2 ||
        scan_wide[0] != (wchar_t)'O' || scan_wide[1] != (wchar_t)'K' ||
        scan_wide[2] != 0 || scan_char != (wchar_t)'Q') {
        return fail((FILE *)0, 46);
    }

    errno = 0;
    scan_narrow[0] = '\0';
    if (swscanf(invalid_wide + 1, L"%1s", scan_narrow) != EOF ||
        errno != EILSEQ) {
        return fail((FILE *)0, 47);
    }

    stream = tmpfile();
    if (stream == (FILE *)0 || fwide(stream, 1) <= 0 ||
        fputws(scan_file_first, stream) < 0 ||
        fputws(scan_file_second, stream) < 0) {
        return fail(stream, 48);
    }
    rewind(stream);
    scan_integer = 0;
    scan_double = 0.0;
    scan_wide[0] = 0;
    scan_char = 0;
    if (fwscanf(stream, L"%d %2ls %lf %lc", &scan_integer, scan_wide,
                &scan_double, &scan_char) != 4 || scan_integer != 17 ||
        scan_wide[0] != (wchar_t)'W' || scan_wide[1] != (wchar_t)'X' ||
        scan_wide[2] != 0 || scan_double != 3.5 ||
        scan_char != (wchar_t)'Q' || fwide(stream, 0) <= 0) {
        return fail(stream, 49);
    }
    scan_second = 0;
    scan_double = 0.0;
    scan_wide[0] = 0;
    scan_char = 0;
    if (call_vfwscanf(stream, L" %d %2ls %lf %lc", &scan_second, scan_wide,
                      &scan_double, &scan_char) != 4 || scan_second != 23 ||
        scan_wide[0] != (wchar_t)'Y' || scan_wide[1] != (wchar_t)'Z' ||
        scan_wide[2] != 0 || scan_double != 4.5 ||
        scan_char != (wchar_t)'R' || fwide(stream, 0) <= 0 ||
        fclose(stream) != 0) {
        return fail((FILE *)0, 50);
    }

    scan_integer = 0;
    scan_second = 0;
    errno = ERANGE;
    if (fwide(stdin, 0) != 0 || wscanf(L"%d", &scan_integer) != 1 ||
        call_vwscanf(L" %d ", &scan_second) != 1 || scan_integer != 12 ||
        scan_second != 34 || getwchar() != (wint_t)'R' ||
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

    if (setlocale(LC_CTYPE, "C.UTF-8") == (char *)0) {
        return fail((FILE *)0, 51);
    }
    stream = fopen(path, "w+");
    if (stream == (FILE *)0 || fwide(stream, 1) <= 0) {
        return fail(stream, 52);
    }
    if (setlocale(LC_CTYPE, "C") == (char *)0 ||
        fputwc((wchar_t)0x20ac, stream) != (wint_t)0x20ac ||
        fputws((const wchar_t[]){(wchar_t)0x1f600, '\n', 0}, stream) < 0 ||
        ftell(stream) != 8L) {
        return fail(stream, 53);
    }
    rewind(stream);
    if (fgetwc(stream) != (wint_t)0x20ac || ftell(stream) != 3L ||
        ungetwc((wint_t)0x20ac, stream) != (wint_t)0x20ac ||
        ftell(stream) != 0L || fgetwc(stream) != (wint_t)0x20ac ||
        ftell(stream) != 3L ||
        fgetws(line, 8, stream) != line ||
        line[0] != (wchar_t)0x1f600 || line[1] != (wchar_t)'\n' ||
        line[2] != 0 || ftell(stream) != 8L || fclose(stream) != 0) {
        return fail((FILE *)0, 54);
    }

    stream = fopen(path, "r");
    if (stream == (FILE *)0 ||
        fread(narrow, 1U, 8U, stream) != 8U ||
        (unsigned char)narrow[0] != 0xe2U ||
        (unsigned char)narrow[1] != 0x82U ||
        (unsigned char)narrow[2] != 0xacU ||
        (unsigned char)narrow[3] != 0xf0U ||
        (unsigned char)narrow[4] != 0x9fU ||
        (unsigned char)narrow[5] != 0x98U ||
        (unsigned char)narrow[6] != 0x80U ||
        narrow[7] != '\n' || fclose(stream) != 0) {
        return fail((FILE *)0, 55);
    }

    stream = fopen(path, "w");
    if (stream == (FILE *)0 ||
        fwrite((const char[]){(char)0xe2, (char)0x82}, 1U, 2U, stream) != 2U ||
        fclose(stream) != 0) {
        return fail((FILE *)0, 56);
    }
    if (setlocale(LC_CTYPE, "C.UTF-8") == (char *)0) {
        return fail((FILE *)0, 57);
    }
    stream = fopen(path, "r");
    errno = ERANGE;
    if (stream == (FILE *)0 || fwide(stream, 1) <= 0 ||
        fgetwc(stream) != WEOF || errno != EILSEQ || !ferror(stream)) {
        return fail(stream, 58);
    }
    clearerr(stream);
    if (fclose(stream) != 0) {
        return fail((FILE *)0, 59);
    }

    if (setlocale(LC_CTYPE, "C") == (char *)0) {
        return fail((FILE *)0, 60);
    }
    stream = tmpfile();
    if (stream == (FILE *)0 || fwide(stream, 1) <= 0 ||
        setlocale(LC_CTYPE, "C.UTF-8") == (char *)0) {
        return fail(stream, 61);
    }
    errno = ERANGE;
    if (fputwc((wchar_t)0x20ac, stream) != WEOF ||
        errno != EILSEQ || !ferror(stream)) {
        return fail(stream, 62);
    }
    clearerr(stream);
    if (fclose(stream) != 0 || setlocale(LC_CTYPE, "C") == (char *)0) {
        return fail((FILE *)0, 63);
    }

    (void)remove(path);
    if (mini_sys_write(1, marker, sizeof(marker) - 1U) !=
        (long)(sizeof(marker) - 1U)) {
        return 34;
    }
    return 0;
}
