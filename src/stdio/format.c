#include <errno.h>
#include <stddef.h>
#include <stdio.h>

#include "stdio_internal.h"

#define MINI_PRINTF_INT_MAX ((unsigned int)(~0U >> 1))
#define MINI_FORMAT_PAD_CHUNK 16U
#define MINI_FLOAT_MAX_PRECISION 9U

struct mini_format_args {
    unsigned long gp[5];
    unsigned long fp[8];
    unsigned long *overflow;
    unsigned int gp_index;
    unsigned int gp_count;
    unsigned int fp_index;
    unsigned int fp_count;
};

_Static_assert(sizeof(unsigned long) == 8U, "formatted ABI requires LP64 long");
_Static_assert(sizeof(void *) == 8U, "formatted ABI requires 64-bit pointers");
_Static_assert(sizeof(unsigned int) == 4U, "formatted ABI requires 32-bit int");
_Static_assert(sizeof(double) == 8U, "formatted ABI requires binary64 double");
_Static_assert(sizeof(struct mini_format_args) == 128U,
               "formatted ABI state size changed");

enum mini_format_length {
    MINI_LEN_NONE,
    MINI_LEN_HH,
    MINI_LEN_H,
    MINI_LEN_L,
    MINI_LEN_LL
};

enum mini_format_sink_kind {
    MINI_FORMAT_SINK_FILE,
    MINI_FORMAT_SINK_MEMORY
};

struct mini_format_sink {
    enum mini_format_sink_kind kind;
    FILE *stream;
    char *buffer;
    size_t size;
    size_t stored;
};

struct mini_format_spec {
    int left;
    int plus;
    int space;
    int alternate;
    int zero;
    unsigned int width;
    int precision_set;
    unsigned int precision;
    enum mini_format_length length;
    char conversion;
};

static int invalid_stream(FILE *stream)
{
    if (stream != (FILE *)0) {
        stream->state |= MINI_FILE_ERROR;
    }
    errno = EINVAL;
    return EOF;
}

static int invalid_format(void)
{
    errno = EINVAL;
    return EOF;
}

static int invalid_buffer(void)
{
    errno = EINVAL;
    return EOF;
}

static unsigned long next_word(struct mini_format_args *args)
{
    unsigned long value;

    if (args->gp_index < args->gp_count) {
        value = args->gp[args->gp_index];
        ++args->gp_index;
        return value;
    }

    value = *args->overflow;
    ++args->overflow;
    return value;
}

static double load_double_bits(const void *address)
{
    double value;
    unsigned char *out = (unsigned char *)&value;
    const unsigned char *in = (const unsigned char *)address;
    size_t i;

    for (i = 0; i < sizeof(value); ++i) {
        out[i] = in[i];
    }
    return value;
}

static unsigned long long double_bits(double value)
{
    unsigned long long bits = 0ULL;
    unsigned char *out = (unsigned char *)&bits;
    const unsigned char *in = (const unsigned char *)&value;
    size_t i;

    for (i = 0; i < sizeof(value); ++i) {
        out[i] = in[i];
    }
    return bits;
}

static double next_double(struct mini_format_args *args)
{
    if (args->fp_index < args->fp_count) {
        double value = load_double_bits(&args->fp[args->fp_index]);
        ++args->fp_index;
        return value;
    }

    {
        double value = load_double_bits(args->overflow);
        ++args->overflow;
        return value;
    }
}

static int word_to_int(unsigned long word)
{
    unsigned int value = (unsigned int)word;
    unsigned int max_value = (~0U) >> 1;

    if (value <= max_value) {
        return (int)value;
    }
    return -1 - (int)(~value);
}

static long long word_to_long_long(unsigned long word)
{
    unsigned long long value = (unsigned long long)word;
    unsigned long long max_value = (~0ULL) >> 1;

    if (value <= max_value) {
        return (long long)value;
    }
    return -1LL - (long long)(~value);
}

static int reserve_count(unsigned int *count, size_t amount)
{
    if (amount > (size_t)(MINI_PRINTF_INT_MAX - *count)) {
        errno = EINVAL;
        return 0;
    }
    *count += (unsigned int)amount;
    return 1;
}

static int sink_write(struct mini_format_sink *sink, const char *bytes,
                      size_t length)
{
    size_t i;

    if (sink->kind == MINI_FORMAT_SINK_FILE) {
        if (__mini_stdio_write(sink->stream, (const unsigned char *)bytes,
                               length) != length) {
            return EOF;
        }
        return 0;
    }

    if (sink->size != 0U && sink->stored < sink->size - 1U) {
        size_t available = (sink->size - 1U) - sink->stored;
        size_t copied = length < available ? length : available;

        for (i = 0; i < copied; ++i) {
            sink->buffer[sink->stored + i] = bytes[i];
        }
        sink->stored += copied;
        sink->buffer[sink->stored] = '\0';
    }
    return 0;
}

static int emit_bytes(struct mini_format_sink *sink, const char *bytes,
                      size_t length, unsigned int *count)
{
    if (!reserve_count(count, length)) {
        return EOF;
    }
    return sink_write(sink, bytes, length);
}

static int emit_repeat(struct mini_format_sink *sink, char byte,
                       unsigned int amount, unsigned int *count)
{
    char pad[MINI_FORMAT_PAD_CHUNK];
    unsigned int i;

    if (amount > MINI_PRINTF_INT_MAX - *count) {
        errno = EINVAL;
        return EOF;
    }

    for (i = 0; i < MINI_FORMAT_PAD_CHUNK; ++i) {
        pad[i] = byte;
    }

    while (amount != 0U) {
        unsigned int chunk = amount;

        if (chunk > MINI_FORMAT_PAD_CHUNK) {
            chunk = MINI_FORMAT_PAD_CHUNK;
        }
        if (emit_bytes(sink, pad, chunk, count) == EOF) {
            return EOF;
        }
        amount -= chunk;
    }
    return 0;
}

static int is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static int parse_decimal(const char **cursor, unsigned int *value)
{
    const char *p = *cursor;
    unsigned int result = 0;

    if (!is_digit(*p)) {
        return 0;
    }

    while (is_digit(*p)) {
        unsigned int digit = (unsigned int)(*p - '0');

        if (result > (MINI_PRINTF_INT_MAX - digit) / 10U) {
            return -1;
        }
        result = result * 10U + digit;
        ++p;
    }

    *cursor = p;
    *value = result;
    return 1;
}

static unsigned int negative_int_width(int value)
{
    return 0U - (unsigned int)value;
}

static int parse_spec(const char **cursor, struct mini_format_spec *spec,
                      struct mini_format_args *args)
{
    const char *p = *cursor;
    int parsed;

    spec->left = 0;
    spec->plus = 0;
    spec->space = 0;
    spec->alternate = 0;
    spec->zero = 0;
    spec->width = 0;
    spec->precision_set = 0;
    spec->precision = 0;
    spec->length = MINI_LEN_NONE;
    spec->conversion = '\0';

    for (;;) {
        if (*p == '-') {
            spec->left = 1;
        } else if (*p == '+') {
            spec->plus = 1;
        } else if (*p == ' ') {
            spec->space = 1;
        } else if (*p == '#') {
            spec->alternate = 1;
        } else if (*p == '0') {
            spec->zero = 1;
        } else {
            break;
        }
        ++p;
    }

    if (*p == '*') {
        int width = word_to_int(next_word(args));

        ++p;
        if (width < 0) {
            spec->left = 1;
            spec->width = negative_int_width(width);
            if (spec->width > MINI_PRINTF_INT_MAX) {
                return 0;
            }
        } else {
            spec->width = (unsigned int)width;
        }
    } else {
        parsed = parse_decimal(&p, &spec->width);
        if (parsed < 0) {
            return 0;
        }
    }

    if (*p == '.') {
        ++p;
        spec->precision_set = 1;
        if (*p == '*') {
            int precision = word_to_int(next_word(args));

            ++p;
            if (precision < 0) {
                spec->precision_set = 0;
            } else {
                spec->precision = (unsigned int)precision;
            }
        } else {
            parsed = parse_decimal(&p, &spec->precision);
            if (parsed < 0) {
                return 0;
            }
            if (parsed == 0) {
                spec->precision = 0;
            }
        }
    }

    if (*p == 'h') {
        ++p;
        if (*p == 'h') {
            ++p;
            spec->length = MINI_LEN_HH;
        } else {
            spec->length = MINI_LEN_H;
        }
    } else if (*p == 'l') {
        ++p;
        if (*p == 'l') {
            ++p;
            spec->length = MINI_LEN_LL;
        } else {
            spec->length = MINI_LEN_L;
        }
    }

    if (*p == '\0') {
        return 0;
    }

    spec->conversion = *p++;
    *cursor = p;
    return 1;
}

static size_t string_length_limit(const char *s, int precision_set,
                                  unsigned int precision)
{
    size_t length = 0;

    while (s[length] != '\0' &&
           (!precision_set || length < (size_t)precision)) {
        ++length;
    }
    return length;
}

static int emit_string(struct mini_format_sink *sink,
                       const struct mini_format_spec *spec, const char *value,
                       unsigned int *count)
{
    static const char null_text[] = "(null)";
    size_t length;
    unsigned int padding = 0;

    if (value == (const char *)0) {
        value = null_text;
    }
    length = string_length_limit(value, spec->precision_set, spec->precision);
    if ((size_t)spec->width > length) {
        padding = spec->width - (unsigned int)length;
    }

    if (!spec->left && emit_repeat(sink, ' ', padding, count) == EOF) {
        return EOF;
    }
    if (emit_bytes(sink, value, length, count) == EOF) {
        return EOF;
    }
    if (spec->left && emit_repeat(sink, ' ', padding, count) == EOF) {
        return EOF;
    }
    return 0;
}

static int emit_character(struct mini_format_sink *sink,
                          const struct mini_format_spec *spec, int value,
                          unsigned int *count)
{
    char byte = (char)(unsigned char)value;
    unsigned int padding = spec->width > 1U ? spec->width - 1U : 0U;

    if (!spec->left && emit_repeat(sink, ' ', padding, count) == EOF) {
        return EOF;
    }
    if (emit_bytes(sink, &byte, 1, count) == EOF) {
        return EOF;
    }
    if (spec->left && emit_repeat(sink, ' ', padding, count) == EOF) {
        return EOF;
    }
    return 0;
}

static size_t make_digits(unsigned long long value, unsigned int base,
                          int uppercase, char *digits)
{
    static const char lower[] = "0123456789abcdef";
    static const char upper[] = "0123456789ABCDEF";
    const char *alphabet = uppercase ? upper : lower;
    size_t length = 0;

    do {
        digits[length++] = alphabet[value % base];
        value /= base;
    } while (value != 0ULL);

    return length;
}

static int emit_number(struct mini_format_sink *sink,
                       const struct mini_format_spec *spec,
                       unsigned long long magnitude, int negative,
                       unsigned int base, int uppercase, int signed_conversion,
                       unsigned int *count)
{
    char digits[64];
    char prefix[3];
    size_t digit_count;
    unsigned int precision_zeros = 0;
    unsigned int zero_padding = 0;
    unsigned int spaces = 0;
    unsigned int prefix_length = 0;
    unsigned int body_length;
    size_t i;

    digit_count = make_digits(magnitude, base, uppercase, digits);
    if (spec->precision_set && spec->precision == 0U && magnitude == 0ULL) {
        digit_count = 0;
    }

    if (signed_conversion) {
        if (negative) {
            prefix[prefix_length++] = '-';
        } else if (spec->plus) {
            prefix[prefix_length++] = '+';
        } else if (spec->space) {
            prefix[prefix_length++] = ' ';
        }
    }

    if (spec->alternate && base == 16U && magnitude != 0ULL) {
        prefix[prefix_length++] = '0';
        prefix[prefix_length++] = uppercase ? 'X' : 'x';
    }

    if (spec->precision_set && spec->precision > digit_count) {
        precision_zeros = spec->precision - (unsigned int)digit_count;
    }
    if (spec->alternate && base == 8U) {
        if (digit_count == 0) {
            digit_count = 1;
            digits[0] = '0';
        } else if (precision_zeros == 0U && digits[digit_count - 1] != '0') {
            precision_zeros = 1;
        }
    }

    body_length = prefix_length + precision_zeros + (unsigned int)digit_count;
    if (spec->width > body_length) {
        unsigned int padding = spec->width - body_length;

        if (spec->zero && !spec->left && !spec->precision_set) {
            zero_padding = padding;
        } else {
            spaces = padding;
        }
    }

    if (!spec->left && emit_repeat(sink, ' ', spaces, count) == EOF) {
        return EOF;
    }
    if (prefix_length != 0U &&
        emit_bytes(sink, prefix, prefix_length, count) == EOF) {
        return EOF;
    }
    if (emit_repeat(sink, '0', zero_padding, count) == EOF ||
        emit_repeat(sink, '0', precision_zeros, count) == EOF) {
        return EOF;
    }
    for (i = digit_count; i != 0; --i) {
        if (emit_bytes(sink, &digits[i - 1], 1, count) == EOF) {
            return EOF;
        }
    }
    if (spec->left && emit_repeat(sink, ' ', spaces, count) == EOF) {
        return EOF;
    }
    return 0;
}

static unsigned long long decimal_scale(unsigned int precision)
{
    static const unsigned long long scales[] = {
        1ULL, 10ULL, 100ULL, 1000ULL, 10000ULL,
        100000ULL, 1000000ULL, 10000000ULL, 100000000ULL, 1000000000ULL
    };

    return scales[precision];
}

static int emit_fixed(struct mini_format_sink *sink,
                      const struct mini_format_spec *spec, double value,
                      unsigned int *count)
{
    unsigned long long bits = double_bits(value);
    unsigned long long exponent = (bits >> 52) & 0x7ffULL;
    unsigned long long mantissa = bits & 0xfffffffffffffULL;
    int negative = (bits >> 63) != 0ULL;
    char prefix[1];
    unsigned int prefix_length = 0;
    unsigned int precision = spec->precision_set ? spec->precision : 6U;
    unsigned int body_length;
    unsigned int spaces = 0;
    unsigned int zero_padding = 0;

    if (spec->length != MINI_LEN_NONE && spec->length != MINI_LEN_L) {
        return invalid_format();
    }
    if (precision > MINI_FLOAT_MAX_PRECISION) {
        return invalid_format();
    }

    if (negative) {
        prefix[prefix_length++] = '-';
    } else if (spec->plus) {
        prefix[prefix_length++] = '+';
    } else if (spec->space) {
        prefix[prefix_length++] = ' ';
    }

    if (exponent == 0x7ffULL) {
        const char *text = mantissa == 0ULL ? "inf" : "nan";

        body_length = prefix_length + 3U;
        if (spec->width > body_length) {
            spaces = spec->width - body_length;
        }
        if (!spec->left && emit_repeat(sink, ' ', spaces, count) == EOF) {
            return EOF;
        }
        if (prefix_length != 0U &&
            emit_bytes(sink, prefix, prefix_length, count) == EOF) {
            return EOF;
        }
        if (emit_bytes(sink, text, 3U, count) == EOF) {
            return EOF;
        }
        if (spec->left && emit_repeat(sink, ' ', spaces, count) == EOF) {
            return EOF;
        }
        return 0;
    }

    if (negative) {
        value = -value;
    }
    if (!(value < 18446744073709551616.0)) {
        return invalid_format();
    }

    {
        unsigned long long whole = (unsigned long long)value;
        unsigned long long scale = decimal_scale(precision);
        double fraction = value - (double)whole;
        double scaled = fraction * (double)scale;
        unsigned long long fractional = (unsigned long long)scaled;
        double remainder = scaled - (double)fractional;
        char whole_digits[64];
        char fraction_digits[MINI_FLOAT_MAX_PRECISION];
        size_t whole_count;
        unsigned int i;
        unsigned int point = precision != 0U || spec->alternate;

        if (remainder > 0.5 ||
            (remainder == 0.5 && (fractional & 1ULL) != 0ULL)) {
            ++fractional;
            if (fractional == scale) {
                fractional = 0ULL;
                ++whole;
            }
        }

        whole_count = make_digits(whole, 10U, 0, whole_digits);
        for (i = precision; i != 0U; --i) {
            fraction_digits[i - 1U] = (char)('0' + (fractional % 10ULL));
            fractional /= 10ULL;
        }

        body_length = prefix_length + (unsigned int)whole_count + point + precision;
        if (spec->width > body_length) {
            unsigned int padding = spec->width - body_length;

            if (spec->zero && !spec->left) {
                zero_padding = padding;
            } else {
                spaces = padding;
            }
        }

        if (!spec->left && emit_repeat(sink, ' ', spaces, count) == EOF) {
            return EOF;
        }
        if (prefix_length != 0U &&
            emit_bytes(sink, prefix, prefix_length, count) == EOF) {
            return EOF;
        }
        if (emit_repeat(sink, '0', zero_padding, count) == EOF) {
            return EOF;
        }
        for (i = (unsigned int)whole_count; i != 0U; --i) {
            if (emit_bytes(sink, &whole_digits[i - 1U], 1U, count) == EOF) {
                return EOF;
            }
        }
        if (point) {
            char dot = '.';

            if (emit_bytes(sink, &dot, 1U, count) == EOF) {
                return EOF;
            }
        }
        if (precision != 0U &&
            emit_bytes(sink, fraction_digits, precision, count) == EOF) {
            return EOF;
        }
        if (spec->left && emit_repeat(sink, ' ', spaces, count) == EOF) {
            return EOF;
        }
    }

    return 0;
}

static long long next_signed(struct mini_format_args *args,
                             enum mini_format_length length)
{
    unsigned long word = next_word(args);

    if (length == MINI_LEN_HH) {
        return (signed char)word_to_int(word);
    }
    if (length == MINI_LEN_H) {
        return (short)word_to_int(word);
    }
    if (length == MINI_LEN_L || length == MINI_LEN_LL) {
        return word_to_long_long(word);
    }
    return word_to_int(word);
}

static unsigned long long next_unsigned(struct mini_format_args *args,
                                        enum mini_format_length length)
{
    unsigned long word = next_word(args);

    if (length == MINI_LEN_HH) {
        return (unsigned char)(unsigned int)word;
    }
    if (length == MINI_LEN_H) {
        return (unsigned short)(unsigned int)word;
    }
    if (length == MINI_LEN_L || length == MINI_LEN_LL) {
        return (unsigned long long)word;
    }
    return (unsigned int)word;
}

static int emit_conversion(struct mini_format_sink *sink,
                           const struct mini_format_spec *spec,
                           struct mini_format_args *args, unsigned int *count)
{
    if (spec->conversion == 's') {
        const char *value;

        if (spec->length != MINI_LEN_NONE) {
            return invalid_format();
        }
        value = (const char *)next_word(args);
        return emit_string(sink, spec, value, count);
    }
    if (spec->conversion == 'c') {
        if (spec->length != MINI_LEN_NONE || spec->precision_set) {
            return invalid_format();
        }
        return emit_character(sink, spec, word_to_int(next_word(args)), count);
    }
    if (spec->conversion == 'd' || spec->conversion == 'i') {
        long long value = next_signed(args, spec->length);
        int negative = value < 0;
        unsigned long long magnitude;

        if (negative) {
            magnitude = 0ULL - (unsigned long long)value;
        } else {
            magnitude = (unsigned long long)value;
        }
        return emit_number(sink, spec, magnitude, negative, 10U, 0, 1, count);
    }
    if (spec->conversion == 'u' || spec->conversion == 'o' ||
        spec->conversion == 'x' || spec->conversion == 'X') {
        unsigned int base = 10U;
        int uppercase = spec->conversion == 'X';
        unsigned long long value = next_unsigned(args, spec->length);

        if (spec->conversion == 'o') {
            base = 8U;
        } else if (spec->conversion == 'x' || spec->conversion == 'X') {
            base = 16U;
        }
        return emit_number(sink, spec, value, 0, base, uppercase, 0, count);
    }
    if (spec->conversion == 'f') {
        return emit_fixed(sink, spec, next_double(args), count);
    }
    if (spec->conversion == '%') {
        char percent = '%';

        if (spec->length != MINI_LEN_NONE || spec->precision_set) {
            return invalid_format();
        }
        return emit_character(sink, spec, percent, count);
    }
    return invalid_format();
}

static int format_dispatch(struct mini_format_sink *sink, const char *format,
                           struct mini_format_args *args)
{
    const char *cursor = format;
    unsigned int count = 0;

    if (format == (const char *)0 || args == (struct mini_format_args *)0) {
        return invalid_format();
    }

    while (*cursor != '\0') {
        const char *literal = cursor;
        struct mini_format_spec spec;

        while (*cursor != '\0' && *cursor != '%') {
            ++cursor;
        }
        if (cursor != literal &&
            emit_bytes(sink, literal, (size_t)(cursor - literal), &count) == EOF) {
            return EOF;
        }
        if (*cursor == '\0') {
            break;
        }

        ++cursor;
        if (!parse_spec(&cursor, &spec, args)) {
            return invalid_format();
        }
        if (emit_conversion(sink, &spec, args, &count) == EOF) {
            return EOF;
        }
    }

    return (int)count;
}

int __mini_format_dispatch(FILE *stream, const char *format,
                           struct mini_format_args *args)
{
    struct mini_format_sink sink;

    if (stream == (FILE *)0 || (stream->mode & MINI_FILE_WRITABLE) == 0U) {
        return invalid_stream(stream);
    }

    sink.kind = MINI_FORMAT_SINK_FILE;
    sink.stream = stream;
    sink.buffer = (char *)0;
    sink.size = 0U;
    sink.stored = 0U;
    return format_dispatch(&sink, format, args);
}

int __mini_snprintf_dispatch(char *buffer, size_t size, const char *format,
                             struct mini_format_args *args)
{
    struct mini_format_sink sink;

    if (size != 0U && buffer == (char *)0) {
        return invalid_buffer();
    }

    sink.kind = MINI_FORMAT_SINK_MEMORY;
    sink.stream = (FILE *)0;
    sink.buffer = buffer;
    sink.size = size;
    sink.stored = 0U;
    if (size != 0U) {
        buffer[0] = '\0';
    }

    return format_dispatch(&sink, format, args);
}