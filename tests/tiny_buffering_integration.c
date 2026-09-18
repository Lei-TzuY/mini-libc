#include <errno.h>
#include <locale.h>
#include <stdarg.h>
#include <stdio.h>
#include <wchar.h>

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

static int tiny_vswprintf(wchar_t *buffer, size_t size,
                          const wchar_t *format, ...)
{
    va_list ap;
    int result;

    va_start(ap, format);
    result = vswprintf(buffer, size, format, ap);
    va_end(ap);
    return result;
}

static int tiny_vfwprintf(FILE *stream, const wchar_t *format, ...)
{
    va_list ap;
    int result;

    va_start(ap, format);
    result = vfwprintf(stream, format, ap);
    va_end(ap);
    return result;
}

static int tiny_vswscanf(const wchar_t *input, const wchar_t *format, ...)
{
    va_list ap;
    int result;

    va_start(ap, format);
    result = vswscanf(input, format, ap);
    va_end(ap);
    return result;
}

static int tiny_vfwscanf(FILE *stream, const wchar_t *format, ...)
{
    va_list ap;
    int result;

    va_start(ap, format);
    result = vfwscanf(stream, format, ap);
    va_end(ap);
    return result;
}

int main(int argc, char **argv)
{
    static const char ok[] = "tiny-buffering-ok\n";
    static const wchar_t wide_text[] = {'W', 'Q', '\n', 0};
    static const wchar_t wide_mem_format[] = {'V', '=', '%', 'd', '/', '%', '.', '1', 'f', 0};
    static const wchar_t wide_mem_expected[] = {'V', '=', '7', '/', '2', '.', '5', 0};
    static const wchar_t wide_hex_format[] = {'%', '#', 'x', 0};
    static const wchar_t wide_hex_expected[] = {'0', 'x', '2', 'a', 0};
    static const wchar_t wide_arg_value[] = {'W', 'X', 0};
    static const wchar_t wide_arg_format[] = {'[', '%', 'l', 's', ':', '%', 'l', 'c', ']', 0};
    static const wchar_t wide_arg_expected[] = {'[', 'W', 'X', ':', 'Q', ']', 0};
    static const wchar_t wide_file_format[] = {'N', '=', '%', 'd', '\n', 0};
    static const wchar_t wide_vfile_format[] = {'V', '=', '%', '.', '1', 'f', '\n', 0};
    static const wchar_t wide_varg_file_format[] = {'W', '=', '%', 'l', 's', '/', '%', 'l', 'c', '\n', 0};
    static const wchar_t wide_file_first[] = {'N', '=', '7', '\n', 0};
    static const wchar_t wide_file_second[] = {'V', '=', '2', '.', '5', '\n', 0};
    static const wchar_t wide_file_third[] = {'W', '=', 'W', 'X', '/', 'Q', '\n', 0};
    static const wchar_t wide_scan_memory[] = {'4', '2', ' ', 'W', 'X', ' ', '2', '.', '5', 0};
    static const wchar_t wide_scan_format[] = {'%', 'd', ' ', '%', '2', 'l', 's', ' ', '%', 'l', 'f', 0};
    static const wchar_t wide_scan_file[] = {'9', ' ', 'Q', '\n', 0};
    static const wchar_t wide_scan_file_format[] = {'%', 'd', ' ', '%', 'l', 'c', 0};
    static const wchar_t utf8_format[] = {
        (wchar_t)0x4e2d, ':', '%', 'l', 's', 0
    };
    static const wchar_t utf8_value[] = {(wchar_t)0x00a2, 0};
    static const wchar_t utf8_expected[] = {
        (wchar_t)0x4e2d, ':', (wchar_t)0x00a2, 0
    };
    char full[4];
    char line[8];
    char standard[BUFSIZ];
    char temporary[4];
    char utf8_narrow[8];
    wchar_t wide_read[16] = {0};
    wchar_t wide_format[32] = {0};
    wchar_t wide_scan_text[4] = {0};
    wchar_t wide_scan_char = 0;
    double wide_scan_double = 0.0;
    int wide_scan_value = 0;
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

    stream = tmpfile();
    if (stream == (FILE *)0 || fwide(stream, 0) != 0 || fwide(stream, 1) <= 0 ||
        fputws(wide_text, stream) < 0 || ftell(stream) != 3L) {
        if (stream != (FILE *)0) {
            fclose(stream);
        }
        return 9;
    }
    errno = 0;
    if (fputc('X', stream) != EOF || errno != EINVAL || !ferror(stream)) {
        fclose(stream);
        return 10;
    }
    clearerr(stream);
    rewind(stream);
    if (fgetws(wide_read, 4, stream) != wide_read ||
        wcscmp(wide_read, wide_text) != 0 || ftell(stream) != 3L ||
        fgetws(wide_read, 4, stream) != (wchar_t *)0 || !feof(stream)) {
        fclose(stream);
        return 11;
    }
    clearerr(stream);
    rewind(stream);
    if (fwide(stream, 0) <= 0 || fgetwc(stream) != (wint_t)'W' ||
        ungetwc((wint_t)'Z', stream) != (wint_t)'Z' || ftell(stream) != 0L ||
        fgetwc(stream) != (wint_t)'Z' || fgetwc(stream) != (wint_t)'Q' ||
        fgetwc(stream) != (wint_t)'\n' || fgetwc(stream) != WEOF ||
        !feof(stream) || fclose(stream) != 0) {
        return 12;
    }

    if (tiny_vswprintf(wide_format, 32U, wide_mem_format, 7, 2.5) != 7 ||
        wcscmp(wide_format, wide_mem_expected) != 0 ||
        swprintf(wide_format, 32U, wide_hex_format, 42U) != 4 ||
        wcscmp(wide_format, wide_hex_expected) != 0 ||
        tiny_vswprintf(wide_format, 32U, wide_arg_format, wide_arg_value,
                       (wint_t)'Q') != 6 ||
        wcscmp(wide_format, wide_arg_expected) != 0) {
        return 13;
    }

    stream = tmpfile();
    if (stream == (FILE *)0 || fwide(stream, 1) <= 0 ||
        fwprintf(stream, wide_file_format, 7) != 4 ||
        tiny_vfwprintf(stream, wide_vfile_format, 2.5) != 6 ||
        tiny_vfwprintf(stream, wide_varg_file_format, wide_arg_value,
                       (wint_t)'Q') != 7 ||
        ftell(stream) != 17L) {
        if (stream != (FILE *)0) {
            fclose(stream);
        }
        return 14;
    }
    rewind(stream);
    if (fgetws(wide_read, 16, stream) != wide_read ||
        wcscmp(wide_read, wide_file_first) != 0 ||
        fgetws(wide_read, 16, stream) != wide_read ||
        wcscmp(wide_read, wide_file_second) != 0 ||
        fgetws(wide_read, 16, stream) != wide_read ||
        wcscmp(wide_read, wide_file_third) != 0 || fclose(stream) != 0) {
        return 15;
    }

    if (setlocale(LC_CTYPE, "C.UTF-8") == (char *)0 ||
        tiny_vswprintf(wide_format, 32U, utf8_format, utf8_value) != 3 ||
        wcscmp(wide_format, utf8_expected) != 0 ||
        snprintf(utf8_narrow, sizeof(utf8_narrow), "%ls", utf8_value) != 2 ||
        (unsigned char)utf8_narrow[0] != 0xc2U ||
        (unsigned char)utf8_narrow[1] != 0xa2U ||
        utf8_narrow[2] != '\0') {
        return 20;
    }
    stream = tmpfile();
    if (stream == (FILE *)0 || fwide(stream, 1) <= 0 ||
        setlocale(LC_CTYPE, "C") == (char *)0 ||
        tiny_vfwprintf(stream, utf8_format, utf8_value) != 3 ||
        ftell(stream) != 6L) {
        if (stream != (FILE *)0) {
            fclose(stream);
        }
        return 21;
    }
    rewind(stream);
    if (fgetws(wide_read, 16, stream) != wide_read ||
        wcscmp(wide_read, utf8_expected) != 0 || fclose(stream) != 0) {
        return 22;
    }

    wide_scan_value = 0;
    wide_scan_text[0] = 0;
    wide_scan_double = 0.0;
    if (tiny_vswscanf(wide_scan_memory, wide_scan_format, &wide_scan_value,
                      wide_scan_text, &wide_scan_double) != 3 ||
        wide_scan_value != 42 || wide_scan_text[0] != (wchar_t)'W' ||
        wide_scan_text[1] != (wchar_t)'X' || wide_scan_text[2] != 0 ||
        wide_scan_double != 2.5) {
        return 16;
    }

    stream = tmpfile();
    if (stream == (FILE *)0 || fwide(stream, 1) <= 0 ||
        fputws(wide_scan_file, stream) < 0) {
        if (stream != (FILE *)0) {
            fclose(stream);
        }
        return 17;
    }
    rewind(stream);
    wide_scan_value = 0;
    wide_scan_char = 0;
    if (tiny_vfwscanf(stream, wide_scan_file_format, &wide_scan_value,
                      &wide_scan_char) != 2 || wide_scan_value != 9 ||
        wide_scan_char != (wchar_t)'Q' || fwide(stream, 0) <= 0 ||
        fclose(stream) != 0) {
        return 18;
    }

    if (fwrite(ok, 1, sizeof(ok) - 1U, stdout) != sizeof(ok) - 1U) {
        return 19;
    }
    return 0;
}
