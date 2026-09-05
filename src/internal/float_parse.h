#ifndef MINI_LIBC_FLOAT_PARSE_H
#define MINI_LIBC_FLOAT_PARSE_H

#define MINI_FLOAT_PARSE_WIDTH_MAX (~0UL)

enum mini_float_parse_policy {
    MINI_FLOAT_PARSE_STRTO,
    MINI_FLOAT_PARSE_SCAN
};

enum mini_float_parse_status {
    MINI_FLOAT_PARSE_MATCH_FAIL = 0,
    MINI_FLOAT_PARSE_SUCCESS = 1,
    MINI_FLOAT_PARSE_INPUT_FAIL = -1,
    MINI_FLOAT_PARSE_RANGE_FAIL = -2
};

struct mini_float_source {
    void *context;
    int (*get)(void *context);
    int (*unget)(int c, void *context);
    void *(*mark)(void *context);
    int (*restore)(void *context, void *mark);
};

int __mini_float_parse(struct mini_float_source *source, unsigned long width,
                       int skip_space, enum mini_float_parse_policy policy,
                       int convert, double *result);
int __mini_float_narrow(double value, float *result);

#endif
