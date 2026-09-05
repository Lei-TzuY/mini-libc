#include <errno.h>
#include <stddef.h>
#include <stdio.h>

#include "stdio_internal.h"

#define MINI_PRINTF_INT_MAX ((unsigned int)(~0U >> 1))
#define MINI_FORMAT_PAD_CHUNK 16U
#define MINI_FLOAT_MAX_PRECISION 9U
#define MINI_FLOAT_HEX_MAX_PRECISION 13U
#define MINI_FLOAT_TEXT_CAPACITY 96U

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

struct mini_float_text {
    char bytes[MINI_FLOAT_TEXT_CAPACITY];
    unsigned int length;
    unsigned int zero_prefix_length;
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

static int float_text_put(struct mini_float_text *text, char byte)
{
    if (text->length >= MINI_FLOAT_TEXT_CAPACITY) {
        return 0;
    }
    text->bytes[text->length++] = byte;
    return 1;
}

static int float_text_uint(struct mini_float_text *text, unsigned int value)
{
    char digits[16];
    unsigned int count = 0;
    unsigned int i;

    if (value == 0U) {
        return float_text_put(text, '0');
    }
    while (value != 0U) {
        digits[count++] = (char)('0' + (value % 10U));
        value /= 10U;
    }
    for (i = count; i != 0U; --i) {
        if (!float_text_put(text, digits[i - 1U])) {
            return 0;
        }
    }
    return 1;
}

static int float_text_exponent(struct mini_float_text *text, char marker,
                               int exponent, int minimum_two_digits)
{
    unsigned int magnitude;

    if (!float_text_put(text, marker)) {
        return 0;
    }
    if (exponent < 0) {
        if (!float_text_put(text, '-')) {
            return 0;
        }
        magnitude = (unsigned int)(-exponent);
    } else {
        if (!float_text_put(text, '+')) {
            return 0;
        }
        magnitude = (unsigned int)exponent;
    }
    if (minimum_two_digits && magnitude < 10U &&
        !float_text_put(text, '0')) {
        return 0;
    }
    return float_text_uint(text, magnitude);
}

static int decimal_normalize(double value, double *normalized, int *exponent)
{
    int exp10 = 0;
    unsigned int steps = 0;

    if (value == 0.0) {
        *normalized = 0.0;
        *exponent = 0;
        return 1;
    }
    while (value >= 10.0 && steps < 400U) {
        value /= 10.0;
        ++exp10;
        ++steps;
    }
    while (value < 1.0 && steps < 800U) {
        value *= 10.0;
        --exp10;
        ++steps;
    }
    if (!(value >= 1.0 && value < 10.0)) {
        return 0;
    }
    *normalized = value;
    *exponent = exp10;
    return 1;
}

static int rounded_significand(double value, unsigned int digits,
                               unsigned long long *result, int *exponent)
{
    double normalized;
    double scaled;
    double remainder;
    unsigned long long magnitude;
    unsigned long long scale;

    if (digits == 0U || digits > MINI_FLOAT_MAX_PRECISION + 1U) {
        return 0;
    }
    if (value == 0.0) {
        *result = 0ULL;
        *exponent = 0;
        return 1;
    }
    if (!decimal_normalize(value, &normalized, exponent)) {
        return 0;
    }

    scale = decimal_scale(digits - 1U);
    scaled = normalized * (double)scale;
    magnitude = (unsigned long long)scaled;
    remainder = scaled - (double)magnitude;
    if (remainder > 0.5 ||
        (remainder == 0.5 && (magnitude & 1ULL) != 0ULL)) {
        ++magnitude;
    }
    if (magnitude == scale * 10ULL) {
        magnitude = scale;
        ++*exponent;
    }
    *result = magnitude;
    return 1;
}

static void fill_decimal_digits(unsigned long long value, unsigned int count,
                                char *digits)
{
    unsigned int i;

    for (i = count; i != 0U; --i) {
        digits[i - 1U] = (char)('0' + (value % 10ULL));
        value /= 10ULL;
    }
}

static int build_fixed_text(struct mini_float_text *text,
                            const struct mini_format_spec *spec, double value)
{
    unsigned int precision = spec->precision_set ? spec->precision : 6U;
    unsigned long long whole;
    unsigned long long scale;
    double fraction;
    double scaled;
    unsigned long long fractional;
    double remainder;
    unsigned long long round_parity;
    char whole_digits[64];
    char fraction_digits[MINI_FLOAT_MAX_PRECISION];
    size_t whole_count;
    unsigned int i;

    if (precision > MINI_FLOAT_MAX_PRECISION ||
        !(value < 18446744073709551616.0)) {
        return 0;
    }

    whole = (unsigned long long)value;
    scale = decimal_scale(precision);
    fraction = value - (double)whole;
    scaled = fraction * (double)scale;
    fractional = (unsigned long long)scaled;
    remainder = scaled - (double)fractional;
    round_parity = precision == 0U ? whole : fractional;

    if (remainder > 0.5 ||
        (remainder == 0.5 && (round_parity & 1ULL) != 0ULL)) {
        ++fractional;
        if (fractional == scale) {
            fractional = 0ULL;
            ++whole;
        }
    }

    whole_count = make_digits(whole, 10U, 0, whole_digits);
    for (i = (unsigned int)whole_count; i != 0U; --i) {
        if (!float_text_put(text, whole_digits[i - 1U])) {
            return 0;
        }
    }
    if (precision != 0U || spec->alternate) {
        if (!float_text_put(text, '.')) {
            return 0;
        }
    }
    for (i = precision; i != 0U; --i) {
        fraction_digits[i - 1U] = (char)('0' + (fractional % 10ULL));
        fractional /= 10ULL;
    }
    for (i = 0U; i < precision; ++i) {
        if (!float_text_put(text, fraction_digits[i])) {
            return 0;
        }
    }
    return 1;
}

static int build_scientific_text(struct mini_float_text *text,
                                 const struct mini_format_spec *spec,
                                 double value, int uppercase)
{
    unsigned int precision = spec->precision_set ? spec->precision : 6U;
    unsigned long long magnitude;
    int exponent;
    char digits[MINI_FLOAT_MAX_PRECISION + 1U];
    unsigned int i;

    if (precision > MINI_FLOAT_MAX_PRECISION ||
        !rounded_significand(value, precision + 1U, &magnitude, &exponent)) {
        return 0;
    }
    fill_decimal_digits(magnitude, precision + 1U, digits);
    if (!float_text_put(text, digits[0])) {
        return 0;
    }
    if (precision != 0U || spec->alternate) {
        if (!float_text_put(text, '.')) {
            return 0;
        }
    }
    for (i = 1U; i <= precision; ++i) {
        if (!float_text_put(text, digits[i])) {
            return 0;
        }
    }
    return float_text_exponent(text, uppercase ? 'E' : 'e', exponent, 1);
}

static int build_general_text(struct mini_float_text *text,
                              const struct mini_format_spec *spec,
                              double value, int uppercase)
{
    unsigned int precision = spec->precision_set ? spec->precision : 6U;
    unsigned long long magnitude;
    int exponent;
    char digits[MINI_FLOAT_MAX_PRECISION];
    unsigned int last;
    unsigned int i;
    int scientific;
    int point;

    if (precision == 0U) {
        precision = 1U;
    }
    if (precision > MINI_FLOAT_MAX_PRECISION ||
        !rounded_significand(value, precision, &magnitude, &exponent)) {
        return 0;
    }
    fill_decimal_digits(magnitude, precision, digits);
    last = precision;
    if (!spec->alternate) {
        while (last > 1U && digits[last - 1U] == '0') {
            --last;
        }
    }

    scientific = exponent < -4 || exponent >= (int)precision;
    if (scientific) {
        if (!float_text_put(text, digits[0])) {
            return 0;
        }
        if (last > 1U || spec->alternate) {
            if (!float_text_put(text, '.')) {
                return 0;
            }
        }
        for (i = 1U; i < last; ++i) {
            if (!float_text_put(text, digits[i])) {
                return 0;
            }
        }
        return float_text_exponent(text, uppercase ? 'E' : 'e', exponent, 1);
    }

    point = exponent + 1;
    if (point <= 0) {
        if (!float_text_put(text, '0') || !float_text_put(text, '.')) {
            return 0;
        }
        for (i = 0U; i < (unsigned int)(-point); ++i) {
            if (!float_text_put(text, '0')) {
                return 0;
            }
        }
        for (i = 0U; i < last; ++i) {
            if (!float_text_put(text, digits[i])) {
                return 0;
            }
        }
        return 1;
    }

    for (i = 0U; i < (unsigned int)point; ++i) {
        char digit = i < last ? digits[i] : '0';

        if (!float_text_put(text, digit)) {
            return 0;
        }
    }
    if ((unsigned int)point < last || spec->alternate) {
        if (!float_text_put(text, '.')) {
            return 0;
        }
    }
    for (i = (unsigned int)point; i < last; ++i) {
        if (!float_text_put(text, digits[i])) {
            return 0;
        }
    }
    return 1;
}

static char hex_digit(unsigned int value, int uppercase)
{
    if (value < 10U) {
        return (char)('0' + value);
    }
    return (char)((uppercase ? 'A' : 'a') + (value - 10U));
}

static int build_hex_text(struct mini_float_text *text,
                          const struct mini_format_spec *spec,
                          unsigned long long bits, int uppercase)
{
    unsigned long long raw_exponent = (bits >> 52) & 0x7ffULL;
    unsigned long long mantissa = bits & 0xfffffffffffffULL;
    unsigned long long significand;
    unsigned long long kept;
    int exponent;
    unsigned int precision;
    unsigned int i;

    if (spec->precision_set) {
        if (spec->precision > MINI_FLOAT_HEX_MAX_PRECISION) {
            return 0;
        }
        precision = spec->precision;
    } else {
        precision = MINI_FLOAT_HEX_MAX_PRECISION;
    }

    if (raw_exponent == 0ULL) {
        significand = mantissa;
        exponent = mantissa == 0ULL ? 0 : -1022;
    } else {
        significand = (1ULL << 52) | mantissa;
        exponent = (int)raw_exponent - 1023;
    }

    if (precision < MINI_FLOAT_HEX_MAX_PRECISION) {
        unsigned int shift = 52U - 4U * precision;
        unsigned long long mask = (1ULL << shift) - 1ULL;
        unsigned long long remainder = significand & mask;
        unsigned long long half = 1ULL << (shift - 1U);

        kept = significand >> shift;
        if (remainder > half ||
            (remainder == half && (kept & 1ULL) != 0ULL)) {
            ++kept;
        }
    } else {
        kept = significand;
    }

    if (!spec->precision_set) {
        while (precision != 0U && (kept & 0xfULL) == 0ULL) {
            kept >>= 4;
            --precision;
        }
    }

    if (!float_text_put(text, '0') ||
        !float_text_put(text, uppercase ? 'X' : 'x')) {
        return 0;
    }
    text->zero_prefix_length = 2U;
    if (!float_text_put(text, hex_digit((unsigned int)(kept >> (4U * precision)),
                                        uppercase))) {
        return 0;
    }
    if (precision != 0U || spec->alternate) {
        if (!float_text_put(text, '.')) {
            return 0;
        }
    }
    for (i = precision; i != 0U; --i) {
        unsigned int nibble = (unsigned int)((kept >> (4U * (i - 1U))) & 0xfULL);

        if (!float_text_put(text, hex_digit(nibble, uppercase))) {
            return 0;
        }
    }
    return float_text_exponent(text, uppercase ? 'P' : 'p', exponent, 0);
}

static int emit_float_text(struct mini_format_sink *sink,
                           const struct mini_format_spec *spec,
                           const struct mini_float_text *text, int negative,
                           int special, unsigned int *count)
{
    char sign = '\0';
    unsigned int sign_length = 0U;
    unsigned int body_length;
    unsigned int spaces = 0U;
    unsigned int zero_padding = 0U;

    if (negative) {
        sign = '-';
    } else if (spec->plus) {
        sign = '+';
    } else if (spec->space) {
        sign = ' ';
    }
    if (sign != '\0') {
        sign_length = 1U;
    }
    body_length = sign_length + text->length;
    if (spec->width > body_length) {
        unsigned int padding = spec->width - body_length;

        if (spec->zero && !spec->left && !special) {
            zero_padding = padding;
        } else {
            spaces = padding;
        }
    }

    if (!spec->left && emit_repeat(sink, ' ', spaces, count) == EOF) {
        return EOF;
    }
    if (sign_length != 0U && emit_bytes(sink, &sign, 1U, count) == EOF) {
        return EOF;
    }
    if (text->zero_prefix_length != 0U &&
        emit_bytes(sink, text->bytes, text->zero_prefix_length, count) == EOF) {
        return EOF;
    }
    if (emit_repeat(sink, '0', zero_padding, count) == EOF) {
        return EOF;
    }
    if (text->length > text->zero_prefix_length &&
        emit_bytes(sink, text->bytes + text->zero_prefix_length,
                   text->length - text->zero_prefix_length, count) == EOF) {
        return EOF;
    }
    if (spec->left && emit_repeat(sink, ' ', spaces, count) == EOF) {
        return EOF;
    }
    return 0;
}

static int emit_float(struct mini_format_sink *sink,
                      const struct mini_format_spec *spec, double value,
                      unsigned int *count)
{
    unsigned long long bits = double_bits(value);
    unsigned long long raw_exponent = (bits >> 52) & 0x7ffULL;
    unsigned long long mantissa = bits & 0xfffffffffffffULL;
    int negative = (bits >> 63) != 0ULL;
    int uppercase = spec->conversion == 'F' || spec->conversion == 'E' ||
                    spec->conversion == 'G' || spec->conversion == 'A';
    struct mini_float_text text;
    int built = 0;

    if (spec->length != MINI_LEN_NONE && spec->length != MINI_LEN_L) {
        return invalid_format();
    }
    text.length = 0U;
    text.zero_prefix_length = 0U;

    if (raw_exponent == 0x7ffULL) {
        const char *special = mantissa == 0ULL
                                  ? (uppercase ? "INF" : "inf")
                                  : (uppercase ? "NAN" : "nan");
        unsigned int i;

        for (i = 0U; i < 3U; ++i) {
            if (!float_text_put(&text, special[i])) {
                return invalid_format();
            }
        }
        return emit_float_text(sink, spec, &text, negative, 1, count);
    }

    if (negative) {
        value = -value;
    }
    if (spec->conversion == 'f' || spec->conversion == 'F') {
        built = build_fixed_text(&text, spec, value);
    } else if (spec->conversion == 'e' || spec->conversion == 'E') {
        built = build_scientific_text(&text, spec, value, uppercase);
    } else if (spec->conversion == 'g' || spec->conversion == 'G') {
        built = build_general_text(&text, spec, value, uppercase);
    } else if (spec->conversion == 'a' || spec->conversion == 'A') {
        built = build_hex_text(&text, spec, bits & 0x7fffffffffffffffULL,
                               uppercase);
    }
    if (!built) {
        return invalid_format();
    }
    return emit_float_text(sink, spec, &text, negative, 0, count);
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
    if (spec->conversion == 'f' || spec->conversion == 'F' ||
        spec->conversion == 'e' || spec->conversion == 'E' ||
        spec->conversion == 'g' || spec->conversion == 'G' ||
        spec->conversion == 'a' || spec->conversion == 'A') {
        return emit_float(sink, spec, next_double(args), count);
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
