#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

static const unsigned char input[] =
    "1 2 3 4 5 6 word Q % skip 12X 0x2a 077 ff abcXYZ42_tail! @ ]-";
static size_t input_offset;
static size_t read_calls;

static int test_vscanf(const char *format, ...)
{
    va_list ap;
    int result;

    va_start(ap, format);
    result = vscanf(format, ap);
    va_end(ap);
    return result;
}

static int test_vfscanf(FILE *stream, const char *format, ...)
{
    va_list ap;
    int result;

    va_start(ap, format);
    result = vfscanf(stream, format, ap);
    va_end(ap);
    return result;
}

static int test_vsscanf(const char *source, const char *format, ...)
{
    va_list ap;
    int result;

    va_start(ap, format);
    result = vsscanf(source, format, ap);
    va_end(ap);
    return result;
}

long mini_test_read(int fd, void *buf, unsigned long count)
{
    unsigned char *out = (unsigned char *)buf;
    size_t remaining;
    size_t amount;
    size_t i;

    if (fd != 0) {
        return -EINVAL;
    }

    ++read_calls;
    if (input_offset == sizeof(input) - 1U) {
        return 0;
    }

    remaining = (sizeof(input) - 1U) - input_offset;
    amount = remaining;
    if (amount > (size_t)count) {
        amount = (size_t)count;
    }
    for (i = 0; i < amount; ++i) {
        out[i] = input[input_offset + i];
    }
    input_offset += amount;
    return (long)amount;
}

long mini_test_write(int fd, const void *buf, unsigned long count)
{
    (void)fd;
    (void)buf;
    (void)count;
    return -EIO;
}

int main(void)
{
    int values[6] = {0, 0, 0, 0, 0, 0};
    int first = 0;
    int second = 1234;
    int auto_hex = 0;
    int auto_oct = 0;
    int memory_auto = 0;
    int memory_oct = 0;
    int memory_last = 0;
    int memory_fail = 91;
    unsigned int hex = 0;
    unsigned int memory_hex = 0;
    char word[5];
    char lower[4];
    char tail[6];
    char literal_set[3];
    char memory_letters[3];
    char mismatch[2] = {'?', '\0'};
    char character = '\0';
    char float_tail = '?';
    float memory_float = 0.0f;
    float memory_float_two = 0.0f;
    float width_float = 0.0f;
    float range_float = 7.0f;
    double memory_double = 0.0;
    double memory_double_two = 0.0;
    double range_double = 7.0;
    size_t calls_before_memory;

    errno = ERANGE;
    if (test_vscanf("%d %d %d %d %d %d",
                    &values[0], &values[1], &values[2], &values[3], &values[4],
                    &values[5]) != 6 ||
        values[0] != 1 || values[1] != 2 || values[2] != 3 ||
        values[3] != 4 || values[4] != 5 || values[5] != 6 ||
        errno != ERANGE || read_calls != 1) {
        return 1;
    }

    if (test_vfscanf(stdin, " %4s %c %% %*s", word, &character) != 2 ||
        word[0] != 'w' || word[1] != 'o' || word[2] != 'r' ||
        word[3] != 'd' || word[4] != '\0' || character != 'Q' ||
        read_calls != 1) {
        return 2;
    }

    if (scanf("%d%d", &first, &second) != 1 || first != 12 ||
        second != 1234 || read_calls != 1) {
        return 3;
    }
    if (getchar() != 'X' || read_calls != 1) {
        return 4;
    }

    if (scanf(" %i %i %x %3[a-z]%*[A-Z0-9]%5[^!]%*1[!]",
              &auto_hex, &auto_oct, &hex, lower, tail) != 5 ||
        auto_hex != 42 || auto_oct != 63 || hex != 255U ||
        lower[0] != 'a' || lower[1] != 'b' || lower[2] != 'c' ||
        lower[3] != '\0' || tail[0] != '_' || tail[1] != 't' ||
        tail[2] != 'a' || tail[3] != 'i' || tail[4] != 'l' ||
        tail[5] != '\0' || read_calls != 1) {
        return 5;
    }

    if (scanf(" %1[a-z]", mismatch) != 0 || mismatch[0] != '?' ||
        getchar() != '@' || read_calls != 1) {
        return 6;
    }
    if (scanf(" %2[]-]", literal_set) != 1 || literal_set[0] != ']' ||
        literal_set[1] != '-' || literal_set[2] != '\0' || read_calls != 1) {
        return 7;
    }

    calls_before_memory = read_calls;
    errno = EIO;
    if (test_vsscanf("0x2a 077 AB 89 7", "%i %i %2[A-Z] %x %d",
                     &memory_auto, &memory_oct, memory_letters, &memory_hex,
                     &memory_last) != 5 ||
        memory_auto != 42 || memory_oct != 63 ||
        memory_letters[0] != 'A' || memory_letters[1] != 'B' ||
        memory_letters[2] != '\0' || memory_hex != 0x89U ||
        memory_last != 7 || errno != EIO || read_calls != calls_before_memory) {
        return 8;
    }

    if (sscanf("X", "%d", &memory_fail) != 0 || memory_fail != 91 ||
        read_calls != calls_before_memory) {
        return 9;
    }
    if (sscanf("", "%d", &memory_fail) != EOF || memory_fail != 91 ||
        read_calls != calls_before_memory) {
        return 10;
    }

    errno = EIO;
    if (test_vsscanf("1.5 -2.5e2 .75 6.02E2", "%f %lf %e %lG",
                     &memory_float, &memory_double, &memory_float_two,
                     &memory_double_two) != 4 ||
        memory_float != 1.5f || memory_double != -250.0 ||
        memory_float_two != 0.75f || memory_double_two != 602.0 ||
        errno != EIO || read_calls != calls_before_memory) {
        return 11;
    }

    errno = EIO;
    if (sscanf("1.5X", "%3f%c", &width_float, &float_tail) != 2 ||
        width_float != 1.5f || float_tail != 'X' || errno != EIO ||
        read_calls != calls_before_memory) {
        return 12;
    }

    float_tail = '?';
    errno = EIO;
    if (sscanf("1e999X", "%*f%c", &float_tail) != 1 || float_tail != 'X' ||
        errno != EIO || read_calls != calls_before_memory) {
        return 13;
    }

    range_float = 7.0f;
    errno = EIO;
    if (sscanf("1e9999", "%f", &range_float) != 0 || range_float != 7.0f ||
        errno != ERANGE || read_calls != calls_before_memory) {
        return 14;
    }

    range_double = 7.0;
    errno = EIO;
    if (sscanf("1e-9999", "%lf", &range_double) != 0 || range_double != 7.0 ||
        errno != ERANGE || read_calls != calls_before_memory) {
        return 15;
    }

    first = 99;
    if (scanf("%d", &first) != EOF || first != 99 || !feof(stdin) ||
        ferror(stdin) || read_calls != 2) {
        return 16;
    }
    if (scanf("%d", &first) != EOF || read_calls != 2) {
        return 17;
    }

    return 0;
}
