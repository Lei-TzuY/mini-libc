#include <errno.h>

#include "float_parse.h"

#define MINI_FLOAT_EOF (-1)
#define MINI_FLOAT_DECIMAL_DIGITS 18U
#define MINI_FLOAT_HEX_DIGITS 15U
#define MINI_FLOAT_EXPONENT_CAP 10000000000UL
#define MINI_FLOAT_DOUBLE_MAX 1.7976931348623157e308
#define MINI_FLOAT_DOUBLE_MIN_NORMAL 2.2250738585072014e-308
#define MINI_FLOAT_SINGLE_MAX 3.4028234663852886e38
#define MINI_FLOAT_SINGLE_MIN_NORMAL 1.1754943508222875e-38

struct mini_float_reader {
    struct mini_float_source *source;
    unsigned long remaining;
};

static int ascii_space(int c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' ||
           c == '\f' || c == '\r';
}

static int ascii_digit(int c)
{
    return c >= '0' && c <= '9';
}

static int ascii_hex(int c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

static int ascii_equal_ci(int c, int lower)
{
    return c == lower || c == lower - ('a' - 'A');
}

static int nan_payload_char(int c)
{
    return ascii_digit(c) || (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') || c == '_';
}

static int reader_get(struct mini_float_reader *reader)
{
    int c;

    if (reader->remaining == 0UL) {
        return MINI_FLOAT_EOF;
    }
    c = reader->source->get(reader->source->context);
    if (c != MINI_FLOAT_EOF) {
        --reader->remaining;
    }
    return c;
}

static int reader_unget(struct mini_float_reader *reader, int c)
{
    if (c == MINI_FLOAT_EOF ||
        reader->source->unget(c, reader->source->context) == MINI_FLOAT_EOF) {
        return MINI_FLOAT_EOF;
    }
    ++reader->remaining;
    return c;
}

static int skip_leading_space(struct mini_float_source *source)
{
    for (;;) {
        int c = source->get(source->context);

        if (c == MINI_FLOAT_EOF) {
            return 0;
        }
        if (!ascii_space(c)) {
            if (source->unget(c, source->context) == MINI_FLOAT_EOF) {
                return -1;
            }
            return 1;
        }
    }
}

static double double_from_bits(unsigned long long bits)
{
    double value;
    unsigned char *out = (unsigned char *)&value;
    const unsigned char *in = (const unsigned char *)&bits;
    unsigned int i;

    for (i = 0U; i < 8U; ++i) {
        out[i] = in[i];
    }
    return value;
}

static unsigned long long double_bits(double value)
{
    unsigned long long bits = 0ULL;
    unsigned char *out = (unsigned char *)&bits;
    const unsigned char *in = (const unsigned char *)&value;
    unsigned int i;

    for (i = 0U; i < 8U; ++i) {
        out[i] = in[i];
    }
    return bits;
}

static double signed_infinity(int negative)
{
    unsigned long long bits = 0x7ff0000000000000ULL;

    if (negative) {
        bits |= 0x8000000000000000ULL;
    }
    return double_from_bits(bits);
}

static double signed_nan(int negative)
{
    unsigned long long bits = 0x7ff8000000000000ULL;

    if (negative) {
        bits |= 0x8000000000000000ULL;
    }
    return double_from_bits(bits);
}

static void accumulate_digit(unsigned long long *significand,
                             unsigned int *stored_digits,
                             unsigned long *fractional_digits,
                             unsigned long *discarded_digits,
                             int *significant_started,
                             int *first_discarded,
                             int *discarded_nonzero,
                             unsigned int digit, int after_point,
                             unsigned int digit_limit,
                             unsigned int base)
{
    if (after_point) {
        ++*fractional_digits;
    }
    if (!*significant_started && digit == 0U) {
        return;
    }
    *significant_started = 1;
    if (*stored_digits < digit_limit) {
        *significand = *significand * (unsigned long long)base + digit;
        ++*stored_digits;
        return;
    }
    ++*discarded_digits;
    if (*first_discarded < 0) {
        *first_discarded = (int)digit;
    } else if (digit != 0U) {
        *discarded_nonzero = 1;
    }
}

static void round_significand(unsigned long long *significand,
                              int first_discarded, int discarded_nonzero,
                              unsigned int base)
{
    unsigned int half = base / 2U;

    if (first_discarded > (int)half ||
        (first_discarded == (int)half &&
         (discarded_nonzero || (*significand & 1ULL) != 0ULL))) {
        ++*significand;
    }
}

static void accumulate_exponent(unsigned long *exponent, int *overflow,
                                unsigned int digit)
{
    if (!*overflow) {
        if (*exponent > (MINI_FLOAT_EXPONENT_CAP - digit) / 10UL) {
            *overflow = 1;
        } else {
            *exponent = *exponent * 10UL + (unsigned long)digit;
        }
    }
}

static int finish_range(double value, double *result)
{
    double magnitude = value < 0.0 ? -value : value;

    *result = value;
    if (magnitude != 0.0 && magnitude < MINI_FLOAT_DOUBLE_MIN_NORMAL) {
        errno = ERANGE;
        return MINI_FLOAT_PARSE_RANGE_FAIL;
    }
    return MINI_FLOAT_PARSE_SUCCESS;
}

static int scale_decimal(unsigned long long significand, long exponent,
                         int negative, double *result)
{
    static const double powers[] = {
        1.0e1, 1.0e2, 1.0e4, 1.0e8, 1.0e16,
        1.0e32, 1.0e64, 1.0e128, 1.0e256
    };
    unsigned long magnitude;
    unsigned int index = 0U;
    double value;

    if (significand == 0ULL) {
        value = 0.0;
        *result = negative ? -value : value;
        return MINI_FLOAT_PARSE_SUCCESS;
    }
    if (exponent > 511L) {
        *result = signed_infinity(negative);
        errno = ERANGE;
        return MINI_FLOAT_PARSE_RANGE_FAIL;
    }
    if (exponent < -511L) {
        value = 0.0;
        *result = negative ? -value : value;
        errno = ERANGE;
        return MINI_FLOAT_PARSE_RANGE_FAIL;
    }

    value = (double)(long long)significand;
    magnitude = (unsigned long)(exponent < 0 ? -exponent : exponent);
    while (magnitude != 0UL) {
        if ((magnitude & 1UL) != 0UL) {
            if (exponent > 0) {
                if (value > MINI_FLOAT_DOUBLE_MAX / powers[index]) {
                    *result = signed_infinity(negative);
                    errno = ERANGE;
                    return MINI_FLOAT_PARSE_RANGE_FAIL;
                }
                value *= powers[index];
            } else {
                value /= powers[index];
            }
        }
        magnitude >>= 1;
        ++index;
    }
    if (value == 0.0) {
        *result = negative ? -value : value;
        errno = ERANGE;
        return MINI_FLOAT_PARSE_RANGE_FAIL;
    }
    if (negative) {
        value = -value;
    }
    return finish_range(value, result);
}

static int scale_binary(unsigned long long significand, long exponent,
                        int negative, double *result)
{
    static const double powers[] = {
        2.0,
        4.0,
        16.0,
        256.0,
        65536.0,
        4294967296.0,
        18446744073709551616.0,
        3.4028236692093846e38,
        1.157920892373162e77,
        1.3407807929942597e154
    };
    unsigned long magnitude;
    unsigned int index = 0U;
    double value;

    if (significand == 0ULL) {
        value = 0.0;
        *result = negative ? -value : value;
        return MINI_FLOAT_PARSE_SUCCESS;
    }
    if (exponent > 2047L) {
        *result = signed_infinity(negative);
        errno = ERANGE;
        return MINI_FLOAT_PARSE_RANGE_FAIL;
    }
    if (exponent < -2047L) {
        value = 0.0;
        *result = negative ? -value : value;
        errno = ERANGE;
        return MINI_FLOAT_PARSE_RANGE_FAIL;
    }

    value = (double)(long long)significand;
    magnitude = (unsigned long)(exponent < 0 ? -exponent : exponent);
    while (magnitude != 0UL) {
        if ((magnitude & 1UL) != 0UL) {
            if (index >= sizeof(powers) / sizeof(powers[0])) {
                if (exponent > 0) {
                    *result = signed_infinity(negative);
                } else {
                    value = 0.0;
                    *result = negative ? -value : value;
                }
                errno = ERANGE;
                return MINI_FLOAT_PARSE_RANGE_FAIL;
            }
            if (exponent > 0) {
                if (value > MINI_FLOAT_DOUBLE_MAX / powers[index]) {
                    *result = signed_infinity(negative);
                    errno = ERANGE;
                    return MINI_FLOAT_PARSE_RANGE_FAIL;
                }
                value *= powers[index];
            } else {
                value /= powers[index];
            }
        }
        magnitude >>= 1;
        ++index;
    }
    if (value == 0.0) {
        *result = negative ? -value : value;
        errno = ERANGE;
        return MINI_FLOAT_PARSE_RANGE_FAIL;
    }
    if (negative) {
        value = -value;
    }
    return finish_range(value, result);
}

static int parse_exponent(struct mini_float_reader *reader, int marker,
                          enum mini_float_parse_policy policy,
                          int *negative, unsigned long *value,
                          int *overflow)
{
    void *checkpoint = (void *)0;
    unsigned long saved_remaining = reader->remaining;
    unsigned int digits = 0U;
    int c;

    if (policy == MINI_FLOAT_PARSE_STRTO && reader->source->mark != 0 &&
        reader->source->restore != 0) {
        if (reader_unget(reader, marker) == MINI_FLOAT_EOF) {
            return MINI_FLOAT_PARSE_INPUT_FAIL;
        }
        checkpoint = reader->source->mark(reader->source->context);
        saved_remaining = reader->remaining;
        marker = reader_get(reader);
        (void)marker;
    }

    c = reader_get(reader);
    if (c == MINI_FLOAT_EOF) {
        if (checkpoint != (void *)0) {
            reader->source->restore(reader->source->context, checkpoint);
            reader->remaining = saved_remaining;
            return MINI_FLOAT_PARSE_SUCCESS;
        }
        return MINI_FLOAT_PARSE_MATCH_FAIL;
    }
    if (c == '+' || c == '-') {
        *negative = c == '-';
        c = reader_get(reader);
        if (c == MINI_FLOAT_EOF) {
            if (checkpoint != (void *)0) {
                reader->source->restore(reader->source->context, checkpoint);
                reader->remaining = saved_remaining;
                return MINI_FLOAT_PARSE_SUCCESS;
            }
            return MINI_FLOAT_PARSE_MATCH_FAIL;
        }
    }

    while (ascii_digit(c)) {
        accumulate_exponent(value, overflow, (unsigned int)(c - '0'));
        ++digits;
        c = reader_get(reader);
        if (c == MINI_FLOAT_EOF) {
            break;
        }
    }
    if (digits == 0U) {
        if (checkpoint != (void *)0) {
            reader->source->restore(reader->source->context, checkpoint);
            reader->remaining = saved_remaining;
            return MINI_FLOAT_PARSE_SUCCESS;
        }
        if (c != MINI_FLOAT_EOF && reader_unget(reader, c) == MINI_FLOAT_EOF) {
            return MINI_FLOAT_PARSE_INPUT_FAIL;
        }
        return MINI_FLOAT_PARSE_MATCH_FAIL;
    }
    if (c != MINI_FLOAT_EOF && reader_unget(reader, c) == MINI_FLOAT_EOF) {
        return MINI_FLOAT_PARSE_INPUT_FAIL;
    }
    return MINI_FLOAT_PARSE_SUCCESS;
}

static int parse_decimal(struct mini_float_reader *reader, int first,
                         enum mini_float_parse_policy policy, int negative,
                         int convert, double *result)
{
    unsigned long long significand = 0ULL;
    unsigned int stored_digits = 0U;
    unsigned int digits = 0U;
    unsigned long fractional_digits = 0UL;
    unsigned long discarded_digits = 0UL;
    unsigned long explicit_exponent = 0UL;
    int significant_started = 0;
    int first_discarded = -1;
    int discarded_nonzero = 0;
    int after_point = 0;
    int exponent_negative = 0;
    int exponent_overflow = 0;
    int c = first;
    long exponent;
    int status;

    for (;;) {
        if (ascii_digit(c)) {
            accumulate_digit(&significand, &stored_digits, &fractional_digits,
                             &discarded_digits, &significant_started,
                             &first_discarded, &discarded_nonzero,
                             (unsigned int)(c - '0'), after_point,
                             MINI_FLOAT_DECIMAL_DIGITS, 10U);
            ++digits;
        } else if (c == '.' && !after_point) {
            after_point = 1;
        } else {
            break;
        }
        c = reader_get(reader);
        if (c == MINI_FLOAT_EOF) {
            break;
        }
    }

    if (digits == 0U) {
        if (c != MINI_FLOAT_EOF && reader_unget(reader, c) == MINI_FLOAT_EOF) {
            return MINI_FLOAT_PARSE_INPUT_FAIL;
        }
        return MINI_FLOAT_PARSE_MATCH_FAIL;
    }

    if (c == 'e' || c == 'E') {
        status = parse_exponent(reader, c, policy, &exponent_negative,
                                &explicit_exponent, &exponent_overflow);
        if (status == MINI_FLOAT_PARSE_MATCH_FAIL ||
            status == MINI_FLOAT_PARSE_INPUT_FAIL) {
            return status;
        }
    } else if (c != MINI_FLOAT_EOF &&
               reader_unget(reader, c) == MINI_FLOAT_EOF) {
        return MINI_FLOAT_PARSE_INPUT_FAIL;
    }

    if (!convert) {
        return MINI_FLOAT_PARSE_SUCCESS;
    }
    round_significand(&significand, first_discarded, discarded_nonzero, 10U);
    if (significand == 0ULL) {
        double zero = 0.0;
        *result = negative ? -zero : zero;
        return MINI_FLOAT_PARSE_SUCCESS;
    }
    if (exponent_overflow) {
        if (exponent_negative) {
            double zero = 0.0;
            *result = negative ? -zero : zero;
        } else {
            *result = signed_infinity(negative);
        }
        errno = ERANGE;
        return MINI_FLOAT_PARSE_RANGE_FAIL;
    }
    exponent = (long)discarded_digits - (long)fractional_digits;
    if (exponent_negative) {
        exponent -= (long)explicit_exponent;
    } else {
        exponent += (long)explicit_exponent;
    }
    return scale_decimal(significand, exponent, negative, result);
}

static int parse_hex(struct mini_float_reader *reader,
                     enum mini_float_parse_policy policy, int negative,
                     int convert, double *result)
{
    unsigned long long significand = 0ULL;
    unsigned int stored_digits = 0U;
    unsigned int digits = 0U;
    unsigned long fractional_digits = 0UL;
    unsigned long discarded_digits = 0UL;
    unsigned long explicit_exponent = 0UL;
    int significant_started = 0;
    int first_discarded = -1;
    int discarded_nonzero = 0;
    int after_point = 0;
    int exponent_negative = 0;
    int exponent_overflow = 0;
    int c = reader_get(reader);
    long exponent;
    int status;

    while (c != MINI_FLOAT_EOF) {
        int digit = ascii_hex(c);

        if (digit >= 0) {
            accumulate_digit(&significand, &stored_digits, &fractional_digits,
                             &discarded_digits, &significant_started,
                             &first_discarded, &discarded_nonzero,
                             (unsigned int)digit, after_point,
                             MINI_FLOAT_HEX_DIGITS, 16U);
            ++digits;
        } else if (c == '.' && !after_point) {
            after_point = 1;
        } else {
            break;
        }
        c = reader_get(reader);
    }
    if (digits == 0U) {
        if (c != MINI_FLOAT_EOF && reader_unget(reader, c) == MINI_FLOAT_EOF) {
            return MINI_FLOAT_PARSE_INPUT_FAIL;
        }
        return MINI_FLOAT_PARSE_MATCH_FAIL;
    }

    if (c == 'p' || c == 'P') {
        status = parse_exponent(reader, c, policy, &exponent_negative,
                                &explicit_exponent, &exponent_overflow);
        if (status == MINI_FLOAT_PARSE_MATCH_FAIL ||
            status == MINI_FLOAT_PARSE_INPUT_FAIL) {
            return status;
        }
    } else if (c != MINI_FLOAT_EOF &&
               reader_unget(reader, c) == MINI_FLOAT_EOF) {
        return MINI_FLOAT_PARSE_INPUT_FAIL;
    }

    if (!convert) {
        return MINI_FLOAT_PARSE_SUCCESS;
    }
    round_significand(&significand, first_discarded, discarded_nonzero, 16U);
    if (significand == 0ULL) {
        double zero = 0.0;
        *result = negative ? -zero : zero;
        return MINI_FLOAT_PARSE_SUCCESS;
    }
    if (exponent_overflow) {
        if (exponent_negative) {
            double zero = 0.0;
            *result = negative ? -zero : zero;
        } else {
            *result = signed_infinity(negative);
        }
        errno = ERANGE;
        return MINI_FLOAT_PARSE_RANGE_FAIL;
    }
    exponent = (long)(discarded_digits * 4UL) -
               (long)(fractional_digits * 4UL);
    if (exponent_negative) {
        exponent -= (long)explicit_exponent;
    } else {
        exponent += (long)explicit_exponent;
    }
    return scale_binary(significand, exponent, negative, result);
}

static int match_required(struct mini_float_reader *reader, const char *text)
{
    while (*text != '\0') {
        int c = reader_get(reader);

        if (c == MINI_FLOAT_EOF) {
            return MINI_FLOAT_PARSE_MATCH_FAIL;
        }
        if (!ascii_equal_ci(c, (unsigned char)*text)) {
            if (reader_unget(reader, c) == MINI_FLOAT_EOF) {
                return MINI_FLOAT_PARSE_INPUT_FAIL;
            }
            return MINI_FLOAT_PARSE_MATCH_FAIL;
        }
        ++text;
    }
    return MINI_FLOAT_PARSE_SUCCESS;
}

static int parse_infinity(struct mini_float_reader *reader,
                          enum mini_float_parse_policy policy, int negative,
                          int convert, double *result)
{
    int status = match_required(reader, "nf");
    int c;

    if (status != MINI_FLOAT_PARSE_SUCCESS) {
        return status;
    }
    c = reader_get(reader);
    if (c == MINI_FLOAT_EOF) {
        if (convert) {
            *result = signed_infinity(negative);
        }
        return MINI_FLOAT_PARSE_SUCCESS;
    }
    if (ascii_equal_ci(c, 'i')) {
        void *checkpoint = (void *)0;
        unsigned long saved_remaining = reader->remaining;

        if (policy == MINI_FLOAT_PARSE_STRTO && reader->source->mark != 0 &&
            reader->source->restore != 0) {
            if (reader_unget(reader, c) == MINI_FLOAT_EOF) {
                return MINI_FLOAT_PARSE_INPUT_FAIL;
            }
            checkpoint = reader->source->mark(reader->source->context);
            saved_remaining = reader->remaining;
            c = reader_get(reader);
            (void)c;
        }
        status = match_required(reader, "nity");
        if (status != MINI_FLOAT_PARSE_SUCCESS) {
            if (checkpoint != (void *)0) {
                reader->source->restore(reader->source->context, checkpoint);
                reader->remaining = saved_remaining;
                if (convert) {
                    *result = signed_infinity(negative);
                }
                return MINI_FLOAT_PARSE_SUCCESS;
            }
            return status;
        }
    } else if (reader_unget(reader, c) == MINI_FLOAT_EOF) {
        return MINI_FLOAT_PARSE_INPUT_FAIL;
    }
    if (convert) {
        *result = signed_infinity(negative);
    }
    return MINI_FLOAT_PARSE_SUCCESS;
}

static int parse_nan(struct mini_float_reader *reader,
                     enum mini_float_parse_policy policy, int negative,
                     int convert, double *result)
{
    int status = match_required(reader, "an");
    int c;

    if (status != MINI_FLOAT_PARSE_SUCCESS) {
        return status;
    }
    c = reader_get(reader);
    if (c == MINI_FLOAT_EOF) {
        if (convert) {
            *result = signed_nan(negative);
        }
        return MINI_FLOAT_PARSE_SUCCESS;
    }
    if (c == '(') {
        void *checkpoint = (void *)0;
        unsigned long saved_remaining = reader->remaining;

        if (policy == MINI_FLOAT_PARSE_STRTO && reader->source->mark != 0 &&
            reader->source->restore != 0) {
            if (reader_unget(reader, c) == MINI_FLOAT_EOF) {
                return MINI_FLOAT_PARSE_INPUT_FAIL;
            }
            checkpoint = reader->source->mark(reader->source->context);
            saved_remaining = reader->remaining;
            c = reader_get(reader);
            (void)c;
        }
        c = reader_get(reader);
        while (c != MINI_FLOAT_EOF && nan_payload_char(c)) {
            c = reader_get(reader);
        }
        if (c != ')') {
            if (checkpoint != (void *)0) {
                reader->source->restore(reader->source->context, checkpoint);
                reader->remaining = saved_remaining;
                if (convert) {
                    *result = signed_nan(negative);
                }
                return MINI_FLOAT_PARSE_SUCCESS;
            }
            if (c != MINI_FLOAT_EOF && reader_unget(reader, c) == MINI_FLOAT_EOF) {
                return MINI_FLOAT_PARSE_INPUT_FAIL;
            }
            return MINI_FLOAT_PARSE_MATCH_FAIL;
        }
    } else if (reader_unget(reader, c) == MINI_FLOAT_EOF) {
        return MINI_FLOAT_PARSE_INPUT_FAIL;
    }
    if (convert) {
        *result = signed_nan(negative);
    }
    return MINI_FLOAT_PARSE_SUCCESS;
}

int __mini_float_parse(struct mini_float_source *source, unsigned long width,
                       int skip_space, enum mini_float_parse_policy policy,
                       int convert, double *result)
{
    struct mini_float_reader reader;
    int negative = 0;
    int c;

    if (skip_space) {
        int skipped = skip_leading_space(source);
        if (skipped < 0) {
            return MINI_FLOAT_PARSE_INPUT_FAIL;
        }
    }

    reader.source = source;
    reader.remaining = width;
    c = reader_get(&reader);
    if (c == MINI_FLOAT_EOF) {
        return MINI_FLOAT_PARSE_INPUT_FAIL;
    }
    if (c == '+' || c == '-') {
        negative = c == '-';
        c = reader_get(&reader);
        if (c == MINI_FLOAT_EOF) {
            return MINI_FLOAT_PARSE_MATCH_FAIL;
        }
    }

    if (ascii_equal_ci(c, 'i')) {
        return parse_infinity(&reader, policy, negative, convert, result);
    }
    if (ascii_equal_ci(c, 'n')) {
        return parse_nan(&reader, policy, negative, convert, result);
    }
    if (c == '0') {
        void *checkpoint = (void *)0;
        unsigned long saved_remaining = reader.remaining;
        int next;

        if (policy == MINI_FLOAT_PARSE_STRTO && source->mark != 0 &&
            source->restore != 0) {
            checkpoint = source->mark(source->context);
        }
        next = reader_get(&reader);
        if (next == 'x' || next == 'X') {
            int status = parse_hex(&reader, policy, negative, convert, result);

            if (status != MINI_FLOAT_PARSE_MATCH_FAIL ||
                policy == MINI_FLOAT_PARSE_SCAN) {
                return status;
            }
            if (checkpoint != (void *)0) {
                source->restore(source->context, checkpoint);
                reader.remaining = saved_remaining;
                if (convert) {
                    double zero = 0.0;
                    *result = negative ? -zero : zero;
                }
                return MINI_FLOAT_PARSE_SUCCESS;
            }
            return status;
        }
        if (next != MINI_FLOAT_EOF &&
            reader_unget(&reader, next) == MINI_FLOAT_EOF) {
            return MINI_FLOAT_PARSE_INPUT_FAIL;
        }
    }
    return parse_decimal(&reader, c, policy, negative, convert, result);
}

int __mini_float_narrow(double value, float *result)
{
    unsigned long long bits = double_bits(value);
    unsigned long long exponent = (bits >> 52) & 0x7ffULL;
    double magnitude;
    float narrowed;

    if (exponent == 0x7ffULL) {
        *result = (float)value;
        return MINI_FLOAT_PARSE_SUCCESS;
    }
    magnitude = value < 0.0 ? -value : value;
    narrowed = (float)value;
    *result = narrowed;
    if (magnitude > MINI_FLOAT_SINGLE_MAX ||
        (value != 0.0 && narrowed == 0.0f)) {
        errno = ERANGE;
        return MINI_FLOAT_PARSE_RANGE_FAIL;
    }
    if (narrowed != 0.0f) {
        float narrowed_magnitude = narrowed < 0.0f ? -narrowed : narrowed;

        if (narrowed_magnitude < MINI_FLOAT_SINGLE_MIN_NORMAL) {
            errno = ERANGE;
            return MINI_FLOAT_PARSE_RANGE_FAIL;
        }
    }
    return MINI_FLOAT_PARSE_SUCCESS;
}
