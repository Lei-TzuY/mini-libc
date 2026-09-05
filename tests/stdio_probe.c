#include <errno.h>
#include <mini/syscall.h>
#include <stdarg.h>
#include <stdio.h>

static int same_string(const char *left, const char *right)
{
    size_t i = 0;

    while (left[i] != '\0' && right[i] != '\0') {
        if (left[i] != right[i]) {
            return 0;
        }
        ++i;
    }
    return left[i] == right[i];
}

static double double_from_bits(unsigned long long bits)
{
    double value;
    unsigned char *out = (unsigned char *)&value;
    const unsigned char *in = (const unsigned char *)&bits;
    size_t i;

    for (i = 0; i < sizeof(value); ++i) {
        out[i] = in[i];
    }
    return value;
}

static int probe_vsnprintf(char *buffer, size_t size, const char *format, ...)
{
    va_list ap;
    int result;

    va_start(ap, format);
    result = vsnprintf(buffer, size, format, ap);
    va_end(ap);
    return result;
}

static int probe_vfprintf(FILE *stream, const char *format, ...)
{
    va_list ap;
    int result;

    va_start(ap, format);
    result = vfprintf(stream, format, ap);
    va_end(ap);
    return result;
}

static int probe_vprintf(const char *format, ...)
{
    va_list ap;
    int result;

    va_start(ap, format);
    result = vprintf(format, ap);
    va_end(ap);
    return result;
}

static int probe_va_copy(char *left, size_t left_size, char *right,
                         size_t right_size, const char *format, ...)
{
    va_list ap;
    va_list copy;
    int left_result;
    int right_result;

    va_start(ap, format);
    va_copy(copy, ap);
    left_result = vsnprintf(left, left_size, format, ap);
    right_result = vsnprintf(right, right_size, format, copy);
    va_end(copy);
    va_end(ap);

    if (left_result < 0 || right_result != left_result) {
        return EOF;
    }
    return left_result;
}

static int probe_va_sum(int count, ...)
{
    va_list ap;
    va_list copy;
    int first_copy;
    int total = 0;
    int i;

    va_start(ap, count);
    va_copy(copy, ap);
    first_copy = va_arg(copy, int);
    va_end(copy);

    for (i = 0; i < count; ++i) {
        total += va_arg(ap, int);
    }
    va_end(ap);

    return first_copy == 1 ? total : -1;
}

int main(void)
{
    static const char expected_basic[] =
        "fmt:-42:17:4000000000:11:2a:2A:Z:ok:%\n";
    static const char expected_padding[] =
        "pad:[-00042][xy   ][0x2a][011][abc][    0023][  0007]\n";
    static const char expected_lengths[] =
        "len:-5:250:-30000:60000:-1234567890:4000000000:-5000000000:9000000000\n";
    static const char expected_star[] =
        "star:[12   ][wide][    002a]\n";
    static const char expected_signs[] =
        "sign:[+7][ 7][00000042][0X2A]\n";
    static const char expected_zero_precision[] =
        "edge:[][0][     ]\n";
    static const char expected_minima[] =
        "min:-2147483648:-9223372036854775808\n";
    static const char expected_fprintf_stack[] =
        "fprintf-stack:1:2:3:4:5\n";
    static const char expected_snprintf_stack[] =
        "snprintf-stack:1:2:3:4:5";
    static const char expected_vstack[] = "vstack:1:2:3:4:5";
    static const char expected_copy[] = "copy:11:22:33:44:55";
    static const char expected_float_stack[] =
        "fp:1:2:3:4:5:6:7:8:9";
    static const char expected_float_flags[] =
        "[    1.50][-2.5   ][+4.][-000.0]";
    static const char expected_float_special[] =
        "[    +inf][nan   ][-inf]";
    static const char expected_vfile[] =
        "vf:1:2:3:4:5|vp:1:2:3:4:5:6|pf:1.5|vpf:2.5|vff:1:2:3:4:5:6:7:8:9";
    char memory[64];
    char float_memory[128];
    char truncated[8];
    char one[1];
    char untouched;
    char copy_left[64];
    char copy_right[64];
    FILE *stream;
    FILE *saved_stdout;
    size_t i;
    int result;

    if (stdin == (FILE *)0 || stdout == (FILE *)0 || stderr == (FILE *)0 ||
        stdin == stdout || stdin == stderr || stdout == stderr) {
        return 1;
    }

    errno = ERANGE;
    result = fgetc(stdin);
    if (result != 'x' || errno != ERANGE || feof(stdin) || ferror(stdin)) {
        return 2;
    }

    result = getchar();
    if (result != 'y' || errno != ERANGE || feof(stdin) || ferror(stdin)) {
        return 3;
    }

    result = getc(stdin);
    if (result != EOF || !feof(stdin) || ferror(stdin) || errno != ERANGE) {
        return 4;
    }
    clearerr(stdin);
    if (feof(stdin) || ferror(stdin)) {
        return 5;
    }

    errno = ERANGE;
    result = fgetc(stdout);
    if (result != EOF || !ferror(stdout) || feof(stdout) || errno != EINVAL) {
        return 6;
    }
    clearerr(stdout);
    if (ferror(stdout) || feof(stdout)) {
        return 7;
    }

    errno = ERANGE;
    result = fputc('!', stdin);
    if (result != EOF || !ferror(stdin) || feof(stdin) || errno != EINVAL) {
        return 8;
    }
    clearerr(stdin);

    errno = ERANGE;
    if (fputc('A', stdout) != 'A' || putc(0x142, stdout) != 'B' ||
        putchar('C') != 'C' || fputs("DE", stdout) < 0 ||
        puts("FG") < 0 || errno != ERANGE || ferror(stdout)) {
        return 9;
    }

    if (fputs("stderr-ok", stderr) < 0 || fputc('\n', stderr) != '\n' ||
        errno != ERANGE || ferror(stderr)) {
        return 10;
    }

    errno = ERANGE;
    result = printf("fmt:%d:%i:%u:%o:%x:%X:%c:%s:%%\n",
                    -42, 17, 4000000000U, 9U, 0x2aU, 0x2aU, 'Z', "ok");
    if (result != (int)(sizeof(expected_basic) - 1U) || errno != ERANGE ||
        ferror(stdout)) {
        return 11;
    }

    result = printf("pad:[%+06d][%-5s][%#x][%#o][%.3s][%8.4d][%*.*u]\n",
                    -42, "xy", 0x2aU, 9U, "abcdef", 23, 6, 4, 7U);
    if (result != (int)(sizeof(expected_padding) - 1U) || errno != ERANGE ||
        ferror(stdout)) {
        return 12;
    }

    result = printf("len:%hhd:%hhu:%hd:%hu:%ld:%lu:%lld:%llu\n",
                    -5, 250, -30000, 60000,
                    -1234567890L, 4000000000UL,
                    -5000000000LL, 9000000000ULL);
    if (result != (int)(sizeof(expected_lengths) - 1U) || errno != ERANGE ||
        ferror(stdout)) {
        return 13;
    }

    result = printf("star:[%*d][%.*s][%0*.*x]\n",
                    -5, 12, -1, "wide", 8, 4, 0x2aU);
    if (result != (int)(sizeof(expected_star) - 1U) || errno != ERANGE ||
        ferror(stdout)) {
        return 14;
    }

    result = printf("sign:[%+d][% d][%08u][%#X]\n", 7, 7, 42U, 0x2aU);
    if (result != (int)(sizeof(expected_signs) - 1U) || errno != ERANGE ||
        ferror(stdout)) {
        return 15;
    }

    result = printf("edge:[%.0d][%#.0o][%5.0u]\n", 0, 0U, 0U);
    if (result != (int)(sizeof(expected_zero_precision) - 1U) ||
        errno != ERANGE || ferror(stdout)) {
        return 16;
    }

    result = printf("min:%d:%lld\n",
                    (-2147483647 - 1), (-9223372036854775807LL - 1LL));
    if (result != (int)(sizeof(expected_minima) - 1U) || errno != ERANGE ||
        ferror(stdout)) {
        return 17;
    }

    result = fprintf(stderr, "format-err:%#08x:%-4c\n", 0x2aU, 'Q');
    if (result != 25 || errno != ERANGE || ferror(stderr)) {
        return 18;
    }

    result = fprintf(stderr, "fprintf-stack:%d:%d:%d:%d:%d\n", 1, 2, 3, 4, 5);
    if (result != (int)(sizeof(expected_fprintf_stack) - 1U) ||
        errno != ERANGE || ferror(stderr)) {
        return 19;
    }

    errno = ERANGE;
    result = snprintf(memory, sizeof(memory),
                      "snprintf-stack:%d:%d:%d:%d:%d", 1, 2, 3, 4, 5);
    if (result != (int)(sizeof(expected_snprintf_stack) - 1U) ||
        !same_string(memory, expected_snprintf_stack) || errno != ERANGE ||
        ferror(stdout)) {
        return 60;
    }

    for (i = 0; i < sizeof(truncated); ++i) {
        truncated[i] = '?';
    }
    result = snprintf(truncated, 5U, "abcdef-%d", 42);
    if (result != 9 || truncated[0] != 'a' || truncated[1] != 'b' ||
        truncated[2] != 'c' || truncated[3] != 'd' || truncated[4] != '\0' ||
        truncated[5] != '?' || truncated[6] != '?' || truncated[7] != '?' ||
        errno != ERANGE || ferror(stdout)) {
        return 61;
    }

    one[0] = 'X';
    result = snprintf(one, sizeof(one), "abc");
    if (result != 3 || one[0] != '\0' || errno != ERANGE || ferror(stdout)) {
        return 62;
    }

    untouched = 'Q';
    result = snprintf(&untouched, 0U, "abc%d", 7);
    if (result != 4 || untouched != 'Q' || errno != ERANGE || ferror(stdout)) {
        return 63;
    }

    result = snprintf((char *)0, 0U, "zero:%u", 7U);
    if (result != 6 || errno != ERANGE || ferror(stdout)) {
        return 64;
    }

    errno = ERANGE;
    if (snprintf((char *)0, 1U, "x") != EOF || errno != EINVAL ||
        ferror(stdout)) {
        return 65;
    }

    errno = ERANGE;
    if (probe_va_sum(7, 1, 2, 3, 4, 5, 6, 7) != 28 || errno != ERANGE) {
        return 67;
    }

    result = probe_vsnprintf(memory, sizeof(memory),
                             "vstack:%d:%d:%d:%d:%d", 1, 2, 3, 4, 5);
    if (result != (int)(sizeof(expected_vstack) - 1U) ||
        !same_string(memory, expected_vstack) || errno != ERANGE ||
        ferror(stdout)) {
        return 68;
    }

    result = probe_va_copy(copy_left, sizeof(copy_left),
                           copy_right, sizeof(copy_right),
                           "copy:%u:%u:%u:%u:%u",
                           11U, 22U, 33U, 44U, 55U);
    if (result != (int)(sizeof(expected_copy) - 1U) ||
        !same_string(copy_left, expected_copy) ||
        !same_string(copy_right, expected_copy) || errno != ERANGE) {
        return 69;
    }

    errno = ERANGE;
    result = snprintf(float_memory, sizeof(float_memory),
                      "fp:%.0f:%.0f:%.0f:%.0f:%.0f:%.0f:%.0f:%.0f:%.0f",
                      1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0);
    if (result != (int)(sizeof(expected_float_stack) - 1U) ||
        !same_string(float_memory, expected_float_stack) || errno != ERANGE) {
        return 76;
    }

    result = probe_vsnprintf(float_memory, sizeof(float_memory),
                             "fp:%.0f:%.0f:%.0f:%.0f:%.0f:%.0f:%.0f:%.0f:%.0f",
                             1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0);
    if (result != (int)(sizeof(expected_float_stack) - 1U) ||
        !same_string(float_memory, expected_float_stack) || errno != ERANGE) {
        return 77;
    }

    result = snprintf(float_memory, sizeof(float_memory),
                      "[%8.2f][%-7.1lf][%+#.0f][%06.1f]",
                      1.5, -2.5, 3.5, -0.0);
    if (result != (int)(sizeof(expected_float_flags) - 1U) ||
        !same_string(float_memory, expected_float_flags) || errno != ERANGE) {
        return 78;
    }

    result = snprintf(float_memory, sizeof(float_memory), "%.0f:%.0f", 2.5, 3.5);
    if (result != 3 || !same_string(float_memory, "2:4") || errno != ERANGE) {
        return 79;
    }

    result = snprintf(float_memory, sizeof(float_memory), "[%*.*f]", 8, 2, 1.5);
    if (result != 10 || !same_string(float_memory, "[    1.50]") ||
        errno != ERANGE) {
        return 80;
    }

    result = snprintf(float_memory, sizeof(float_memory), "[%+8f][%-6f][% f]",
                      double_from_bits(0x7ff0000000000000ULL),
                      double_from_bits(0x7ff8000000000000ULL),
                      double_from_bits(0xfff0000000000000ULL));
    if (result != (int)(sizeof(expected_float_special) - 1U) ||
        !same_string(float_memory, expected_float_special) || errno != ERANGE) {
        return 81;
    }

    errno = ERANGE;
    if (snprintf(float_memory, sizeof(float_memory), "%.10f", 1.0) != EOF ||
        errno != EINVAL) {
        return 82;
    }
    errno = ERANGE;
    if (snprintf(float_memory, sizeof(float_memory), "%f",
                 18446744073709551616.0) != EOF || errno != EINVAL) {
        return 83;
    }
    errno = ERANGE;

    stream = fopen("build/vformat-probe.tmp", "w+");
    if (stream == (FILE *)0) {
        return 70;
    }
    result = probe_vfprintf(stream, "vf:%d:%d:%d:%d:%d", 1, 2, 3, 4, 5);
    if (result != 12 || errno != ERANGE || ferror(stream)) {
        fclose(stream);
        return 71;
    }
    saved_stdout = __mini_stdout;
    __mini_stdout = stream;
    result = probe_vprintf("|vp:%d:%d:%d:%d:%d:%d", 1, 2, 3, 4, 5, 6);
    if (result != 15 || errno != ERANGE || ferror(stream)) {
        __mini_stdout = saved_stdout;
        fclose(stream);
        return 72;
    }
    result = printf("|pf:%.1f", 1.5);
    if (result != 7 || errno != ERANGE || ferror(stream)) {
        __mini_stdout = saved_stdout;
        fclose(stream);
        return 84;
    }
    result = probe_vprintf("|vpf:%.1f", 2.5);
    __mini_stdout = saved_stdout;
    if (result != 8 || errno != ERANGE || ferror(stream)) {
        fclose(stream);
        return 85;
    }
    result = probe_vfprintf(stream,
                            "|vff:%.0f:%.0f:%.0f:%.0f:%.0f:%.0f:%.0f:%.0f:%.0f",
                            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0);
    if (result != 22 || errno != ERANGE || ferror(stream)) {
        fclose(stream);
        return 86;
    }
    if (fflush(stream) == EOF) {
        fclose(stream);
        return 73;
    }
    rewind(stream);
    if (fread(memory, 1U, sizeof(expected_vfile) - 1U, stream) !=
            sizeof(expected_vfile) - 1U) {
        fclose(stream);
        return 74;
    }
    memory[sizeof(expected_vfile) - 1U] = '\0';
    if (!same_string(memory, expected_vfile) || fclose(stream) != 0) {
        return 75;
    }

    errno = ERANGE;
    if (printf("%q") != EOF || errno != EINVAL || ferror(stdout)) {
        return 20;
    }
    errno = ERANGE;
    if (printf("%") != EOF || errno != EINVAL || ferror(stdout)) {
        return 21;
    }
    errno = ERANGE;
    if (printf("%*s", (-2147483647 - 1), "x") != EOF ||
        errno != EINVAL || ferror(stdout)) {
        return 22;
    }

    errno = ERANGE;
    if (fflush(stdout) == EOF || errno != ERANGE || ferror(stdout)) {
        return 23;
    }

    {
        static const char marker[] = "stdio-ok";
        long written = mini_sys_write(1, marker, sizeof(marker) - 1);

        if (written != (long)(sizeof(marker) - 1)) {
            return 24;
        }
    }

    if (mini_sys_close(1) != 0) {
        return 25;
    }
    clearerr(stdout);

    errno = ERANGE;
    result = snprintf(memory, sizeof(memory), "closed:%u", 7U);
    if (result != 8 || !same_string(memory, "closed:7") || errno != ERANGE ||
        ferror(stdout)) {
        return 66;
    }

    if (printf("%300s", "x") != EOF || !ferror(stdout)) {
        return 26;
    }

    return 0;
}