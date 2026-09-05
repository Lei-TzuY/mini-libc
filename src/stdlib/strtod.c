#include <stdlib.h>

#include "../internal/float_parse.h"

struct mini_string_float_source {
    const unsigned char *begin;
    const unsigned char *cursor;
};

static int string_get(void *context)
{
    struct mini_string_float_source *source =
        (struct mini_string_float_source *)context;
    int value;

    if (*source->cursor == '\0') {
        return -1;
    }
    value = (int)*source->cursor;
    ++source->cursor;
    return value;
}

static int string_unget(int c, void *context)
{
    struct mini_string_float_source *source =
        (struct mini_string_float_source *)context;

    if (c == -1 || source->cursor == source->begin ||
        source->cursor[-1] != (unsigned char)c) {
        return -1;
    }
    --source->cursor;
    return c;
}

static void *string_mark(void *context)
{
    struct mini_string_float_source *source =
        (struct mini_string_float_source *)context;

    return (void *)source->cursor;
}

static int string_restore(void *context, void *mark)
{
    struct mini_string_float_source *source =
        (struct mini_string_float_source *)context;

    source->cursor = (const unsigned char *)mark;
    return 1;
}

static int parse_string(const char *nptr, char **endptr, double *value)
{
    struct mini_string_float_source state;
    struct mini_float_source source;
    int status;

    state.begin = (const unsigned char *)nptr;
    state.cursor = state.begin;
    source.context = &state;
    source.get = string_get;
    source.unget = string_unget;
    source.mark = string_mark;
    source.restore = string_restore;

    status = __mini_float_parse(&source, MINI_FLOAT_PARSE_WIDTH_MAX, 1,
                                MINI_FLOAT_PARSE_STRTO, 1, value);
    if (status == MINI_FLOAT_PARSE_MATCH_FAIL ||
        status == MINI_FLOAT_PARSE_INPUT_FAIL) {
        *value = 0.0;
        if (endptr != (char **)0) {
            *endptr = (char *)nptr;
        }
        return status;
    }

    if (endptr != (char **)0) {
        *endptr = (char *)state.cursor;
    }
    return status;
}

double strtod(const char *restrict nptr, char **restrict endptr)
{
    double value = 0.0;

    (void)parse_string(nptr, endptr, &value);
    return value;
}

float strtof(const char *restrict nptr, char **restrict endptr)
{
    double value = 0.0;
    float narrowed = 0.0f;
    int status = parse_string(nptr, endptr, &value);

    if (status == MINI_FLOAT_PARSE_MATCH_FAIL ||
        status == MINI_FLOAT_PARSE_INPUT_FAIL) {
        return 0.0f;
    }
    (void)__mini_float_narrow(value, &narrowed);
    return narrowed;
}
