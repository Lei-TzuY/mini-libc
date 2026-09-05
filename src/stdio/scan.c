#include <errno.h>
#include <stdio.h>

#include "stdio_internal.h"

#define MINI_SCAN_WIDTH_MAX (~0U)

struct mini_scan_args {
    unsigned long gp[5];
    unsigned long *overflow;
    unsigned int gp_index;
    unsigned int gp_count;
};

_Static_assert(sizeof(unsigned long) == 8U, "scan ABI requires LP64 long");
_Static_assert(sizeof(void *) == 8U, "scan ABI requires 64-bit pointers");
_Static_assert(sizeof(unsigned int) == 4U, "scan ABI requires 32-bit int");
_Static_assert(sizeof(struct mini_scan_args) == 56U,
               "scan ABI state size changed");

enum mini_scan_length {
    MINI_SCAN_LEN_NONE,
    MINI_SCAN_LEN_HH,
    MINI_SCAN_LEN_H,
    MINI_SCAN_LEN_L,
    MINI_SCAN_LEN_LL
};

struct mini_scan_spec {
    int suppress;
    int width_set;
    unsigned int width;
    enum mini_scan_length length;
    char conversion;
};

enum mini_scan_status {
    MINI_SCAN_MATCH_FAIL = 0,
    MINI_SCAN_SUCCESS = 1,
    MINI_SCAN_INPUT_FAIL = -1,
    MINI_SCAN_RANGE_FAIL = -2
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

static unsigned long next_word(struct mini_scan_args *args)
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

static int is_scan_space(int c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' ||
           c == '\f' || c == '\r';
}

static int is_digit(int c)
{
    return c >= '0' && c <= '9';
}

static int skip_input_space(FILE *stream)
{
    for (;;) {
        int c = fgetc(stream);

        if (c == EOF) {
            return 0;
        }
        if (!is_scan_space(c)) {
            if (ungetc(c, stream) == EOF) {
                return -1;
            }
            return 1;
        }
    }
}

static int parse_width(const char **cursor, unsigned int *value)
{
    const char *p = *cursor;
    unsigned int result = 0;

    if (!is_digit((unsigned char)*p)) {
        return 0;
    }

    while (is_digit((unsigned char)*p)) {
        unsigned int digit = (unsigned int)(*p - '0');

        if (result > (MINI_SCAN_WIDTH_MAX - digit) / 10U) {
            return -1;
        }
        result = result * 10U + digit;
        ++p;
    }
    if (result == 0U) {
        return -1;
    }

    *cursor = p;
    *value = result;
    return 1;
}

static int parse_spec(const char **cursor, struct mini_scan_spec *spec)
{
    const char *p = *cursor;
    int width_result;

    spec->suppress = 0;
    spec->width_set = 0;
    spec->width = 0;
    spec->length = MINI_SCAN_LEN_NONE;
    spec->conversion = '\0';

    if (*p == '*') {
        spec->suppress = 1;
        ++p;
    }

    width_result = parse_width(&p, &spec->width);
    if (width_result < 0) {
        return 0;
    }
    spec->width_set = width_result != 0;

    if (*p == 'h') {
        ++p;
        if (*p == 'h') {
            spec->length = MINI_SCAN_LEN_HH;
            ++p;
        } else {
            spec->length = MINI_SCAN_LEN_H;
        }
    } else if (*p == 'l') {
        ++p;
        if (*p == 'l') {
            spec->length = MINI_SCAN_LEN_LL;
            ++p;
        } else {
            spec->length = MINI_SCAN_LEN_L;
        }
    }

    spec->conversion = *p;
    if (*p == '\0') {
        return 0;
    }
    ++p;

    if (spec->conversion == 'd' || spec->conversion == 'u') {
        *cursor = p;
        return 1;
    }
    if (spec->conversion == 'c' || spec->conversion == 's') {
        if (spec->length != MINI_SCAN_LEN_NONE) {
            return 0;
        }
        *cursor = p;
        return 1;
    }
    if (spec->conversion == '%') {
        if (spec->suppress || spec->width_set ||
            spec->length != MINI_SCAN_LEN_NONE) {
            return 0;
        }
        *cursor = p;
        return 1;
    }

    return 0;
}

static unsigned long long signed_limit(enum mini_scan_length length)
{
    if (length == MINI_SCAN_LEN_HH) {
        return (unsigned long long)(((unsigned char)~0U) >> 1);
    }
    if (length == MINI_SCAN_LEN_H) {
        return (unsigned long long)(((unsigned short)~0U) >> 1);
    }
    if (length == MINI_SCAN_LEN_L) {
        return (unsigned long long)((~0UL) >> 1);
    }
    if (length == MINI_SCAN_LEN_LL) {
        return (~0ULL) >> 1;
    }
    return (unsigned long long)((~0U) >> 1);
}

static unsigned long long unsigned_limit(enum mini_scan_length length)
{
    if (length == MINI_SCAN_LEN_HH) {
        return (unsigned long long)(unsigned char)~0U;
    }
    if (length == MINI_SCAN_LEN_H) {
        return (unsigned long long)(unsigned short)~0U;
    }
    if (length == MINI_SCAN_LEN_L) {
        return (unsigned long long)~0UL;
    }
    if (length == MINI_SCAN_LEN_LL) {
        return ~0ULL;
    }
    return (unsigned long long)~0U;
}

static int assign_signed(struct mini_scan_args *args,
                         enum mini_scan_length length,
                         unsigned long long magnitude, int negative)
{
    unsigned long long limit = signed_limit(length);
    long long value;
    unsigned long word;

    if ((!negative && magnitude > limit) ||
        (negative && magnitude > limit + 1ULL)) {
        errno = ERANGE;
        return 0;
    }

    if (negative) {
        if (magnitude == 0ULL) {
            value = 0;
        } else {
            value = -1LL - (long long)(magnitude - 1ULL);
        }
    } else {
        value = (long long)magnitude;
    }

    word = next_word(args);
    if (length == MINI_SCAN_LEN_HH) {
        *(signed char *)word = (signed char)value;
    } else if (length == MINI_SCAN_LEN_H) {
        *(short *)word = (short)value;
    } else if (length == MINI_SCAN_LEN_L) {
        *(long *)word = (long)value;
    } else if (length == MINI_SCAN_LEN_LL) {
        *(long long *)word = value;
    } else {
        *(int *)word = (int)value;
    }
    return 1;
}

static int assign_unsigned(struct mini_scan_args *args,
                           enum mini_scan_length length,
                           unsigned long long magnitude, int negative)
{
    unsigned long long limit = unsigned_limit(length);
    unsigned long long value;
    unsigned long word;

    if (!negative && magnitude > limit) {
        errno = ERANGE;
        return 0;
    }

    value = negative ? 0ULL - magnitude : magnitude;
    word = next_word(args);
    if (length == MINI_SCAN_LEN_HH) {
        *(unsigned char *)word = (unsigned char)value;
    } else if (length == MINI_SCAN_LEN_H) {
        *(unsigned short *)word = (unsigned short)value;
    } else if (length == MINI_SCAN_LEN_L) {
        *(unsigned long *)word = (unsigned long)value;
    } else if (length == MINI_SCAN_LEN_LL) {
        *(unsigned long long *)word = value;
    } else {
        *(unsigned int *)word = (unsigned int)value;
    }
    return 1;
}

static int scan_decimal(FILE *stream, const struct mini_scan_spec *spec,
                        struct mini_scan_args *args, int signed_conversion)
{
    unsigned int remaining = spec->width_set ? spec->width : MINI_SCAN_WIDTH_MAX;
    unsigned long long magnitude = 0;
    int negative = 0;
    int overflow = 0;
    int c;

    if (skip_input_space(stream) < 0) {
        return MINI_SCAN_INPUT_FAIL;
    }

    c = fgetc(stream);
    if (c == EOF) {
        return MINI_SCAN_INPUT_FAIL;
    }

    if (c == '+' || c == '-') {
        negative = c == '-';
        --remaining;
        if (remaining == 0U) {
            return MINI_SCAN_MATCH_FAIL;
        }
        c = fgetc(stream);
        if (c == EOF) {
            return MINI_SCAN_INPUT_FAIL;
        }
    }

    if (!is_digit(c)) {
        if (ungetc(c, stream) == EOF) {
            return MINI_SCAN_INPUT_FAIL;
        }
        return MINI_SCAN_MATCH_FAIL;
    }

    for (;;) {
        unsigned int digit = (unsigned int)(c - '0');

        if (!overflow) {
            if (magnitude > (~0ULL - digit) / 10ULL) {
                overflow = 1;
            } else {
                magnitude = magnitude * 10ULL + digit;
            }
        }

        --remaining;
        if (remaining == 0U) {
            break;
        }

        c = fgetc(stream);
        if (c == EOF) {
            break;
        }
        if (!is_digit(c)) {
            if (ungetc(c, stream) == EOF) {
                return MINI_SCAN_INPUT_FAIL;
            }
            break;
        }
    }

    if (spec->suppress) {
        return MINI_SCAN_SUCCESS;
    }
    if (overflow) {
        errno = ERANGE;
        return MINI_SCAN_RANGE_FAIL;
    }

    if (signed_conversion) {
        if (!assign_signed(args, spec->length, magnitude, negative)) {
            return MINI_SCAN_RANGE_FAIL;
        }
    } else if (!assign_unsigned(args, spec->length, magnitude, negative)) {
        return MINI_SCAN_RANGE_FAIL;
    }

    return MINI_SCAN_SUCCESS;
}

static int scan_string(FILE *stream, const struct mini_scan_spec *spec,
                       struct mini_scan_args *args)
{
    unsigned int remaining = spec->width_set ? spec->width : MINI_SCAN_WIDTH_MAX;
    char *destination = (char *)0;
    unsigned int count = 0;
    int c;

    if (skip_input_space(stream) < 0) {
        return MINI_SCAN_INPUT_FAIL;
    }

    c = fgetc(stream);
    if (c == EOF) {
        return MINI_SCAN_INPUT_FAIL;
    }
    if (is_scan_space(c)) {
        if (ungetc(c, stream) == EOF) {
            return MINI_SCAN_INPUT_FAIL;
        }
        return MINI_SCAN_MATCH_FAIL;
    }

    if (!spec->suppress) {
        destination = (char *)next_word(args);
    }

    for (;;) {
        if (!spec->suppress) {
            destination[count] = (char)(unsigned char)c;
        }
        ++count;
        --remaining;
        if (remaining == 0U) {
            break;
        }

        c = fgetc(stream);
        if (c == EOF) {
            break;
        }
        if (is_scan_space(c)) {
            if (ungetc(c, stream) == EOF) {
                return MINI_SCAN_INPUT_FAIL;
            }
            break;
        }
    }

    if (!spec->suppress) {
        destination[count] = '\0';
    }
    return MINI_SCAN_SUCCESS;
}

static int scan_characters(FILE *stream, const struct mini_scan_spec *spec,
                           struct mini_scan_args *args)
{
    unsigned int width = spec->width_set ? spec->width : 1U;
    char *destination = (char *)0;
    unsigned int count;

    if (!spec->suppress) {
        destination = (char *)next_word(args);
    }

    for (count = 0; count < width; ++count) {
        int c = fgetc(stream);

        if (c == EOF) {
            return MINI_SCAN_INPUT_FAIL;
        }
        if (!spec->suppress) {
            destination[count] = (char)(unsigned char)c;
        }
    }

    return MINI_SCAN_SUCCESS;
}

static int match_literal(FILE *stream, int expected)
{
    int c = fgetc(stream);

    if (c == EOF) {
        return MINI_SCAN_INPUT_FAIL;
    }
    if (c != expected) {
        if (ungetc(c, stream) == EOF) {
            return MINI_SCAN_INPUT_FAIL;
        }
        return MINI_SCAN_MATCH_FAIL;
    }
    return MINI_SCAN_SUCCESS;
}

static int handle_status(int status, int assignments)
{
    if (status == MINI_SCAN_INPUT_FAIL) {
        return assignments == 0 ? EOF : assignments;
    }
    return assignments;
}

int __mini_scan_dispatch(FILE *stream, const char *format,
                         struct mini_scan_args *args)
{
    const char *cursor = format;
    int assignments = 0;

    if (stream == (FILE *)0 || (stream->mode & MINI_FILE_READABLE) == 0U) {
        return invalid_stream(stream);
    }
    if (format == (const char *)0 || args == (struct mini_scan_args *)0) {
        return invalid_format();
    }

    while (*cursor != '\0') {
        int status;

        if (is_scan_space((unsigned char)*cursor)) {
            while (is_scan_space((unsigned char)*cursor)) {
                ++cursor;
            }
            if (skip_input_space(stream) < 0) {
                return handle_status(MINI_SCAN_INPUT_FAIL, assignments);
            }
            continue;
        }

        if (*cursor != '%') {
            status = match_literal(stream, (unsigned char)*cursor);
            if (status != MINI_SCAN_SUCCESS) {
                return handle_status(status, assignments);
            }
            ++cursor;
            continue;
        }

        {
            struct mini_scan_spec spec;

            ++cursor;
            if (!parse_spec(&cursor, &spec)) {
                return invalid_format();
            }

            if (spec.conversion == '%') {
                status = match_literal(stream, '%');
            } else if (spec.conversion == 'd') {
                status = scan_decimal(stream, &spec, args, 1);
            } else if (spec.conversion == 'u') {
                status = scan_decimal(stream, &spec, args, 0);
            } else if (spec.conversion == 's') {
                status = scan_string(stream, &spec, args);
            } else {
                status = scan_characters(stream, &spec, args);
            }

            if (status != MINI_SCAN_SUCCESS) {
                return handle_status(status, assignments);
            }
            if (spec.conversion != '%' && !spec.suppress) {
                ++assignments;
            }
        }
    }

    return assignments;
}
