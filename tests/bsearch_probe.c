#include <errno.h>
#include <mini/syscall.h>
#include <stddef.h>
#include <stdlib.h>

struct record {
    int key;
    unsigned char payload[3];
};

struct triple {
    unsigned char key;
    unsigned char a;
    unsigned char b;
};

static int compare_int(const void *key_ptr, const void *element_ptr)
{
    int key = *(const int *)key_ptr;
    int element = *(const int *)element_ptr;

    return key < element ? -1 : key > element ? 1 : 0;
}

static int compare_record(const void *key_ptr, const void *element_ptr)
{
    int key = *(const int *)key_ptr;
    int element = ((const struct record *)element_ptr)->key;

    return key < element ? -1 : key > element ? 1 : 0;
}

static int compare_triple(const void *left_ptr, const void *right_ptr)
{
    unsigned char left = ((const struct triple *)left_ptr)->key;
    unsigned char right = ((const struct triple *)right_ptr)->key;

    return left < right ? -1 : left > right ? 1 : 0;
}

static int compare_calls;

static int counting_compare(const void *key_ptr, const void *element_ptr)
{
    (void)key_ptr;
    (void)element_ptr;
    ++compare_calls;
    return 0;
}

static int ints_sorted(const int *values, size_t count)
{
    size_t i;

    for (i = 1; i < count; ++i) {
        if (values[i - 1] > values[i]) {
            return 0;
        }
    }
    return 1;
}

static int triples_valid(const struct triple *values, size_t count)
{
    unsigned int seen_1 = 0;
    unsigned int seen_10 = 0;
    unsigned int seen_12 = 0;
    unsigned int seen_20 = 0;
    unsigned int seen_30 = 0;
    size_t i;

    for (i = 0; i < count; ++i) {
        if (i != 0 && values[i - 1].key > values[i].key) {
            return 0;
        }
        if (values[i].a == 1 && values[i].b == 2 && values[i].key == 0) {
            ++seen_1;
        } else if (values[i].a == 10 && values[i].b == 11 &&
                   values[i].key == 1) {
            ++seen_10;
        } else if (values[i].a == 12 && values[i].b == 13 &&
                   values[i].key == 1) {
            ++seen_12;
        } else if (values[i].a == 20 && values[i].b == 21 &&
                   values[i].key == 2) {
            ++seen_20;
        } else if (values[i].a == 30 && values[i].b == 31 &&
                   values[i].key == 3) {
            ++seen_30;
        } else {
            return 0;
        }
    }

    return seen_1 == 1 && seen_10 == 1 && seen_12 == 1 && seen_20 == 1 &&
           seen_30 == 1;
}

static unsigned int int_magnitude(int value)
{
    unsigned int bits = (unsigned int)value;

    return value < 0 ? 0U - bits : bits;
}

static unsigned long long_magnitude(long value)
{
    unsigned long bits = (unsigned long)value;

    return value < 0 ? 0UL - bits : bits;
}

static unsigned long long long_long_magnitude(long long value)
{
    unsigned long long bits = (unsigned long long)value;

    return value < 0 ? 0ULL - bits : bits;
}

static int div_matches(int numer, int denom, int quot, int rem)
{
    div_t result = div(numer, denom);

    if (result.quot != quot || result.rem != rem) {
        return 0;
    }
    if (result.rem != 0 && ((result.rem < 0) != (numer < 0))) {
        return 0;
    }
    return int_magnitude(result.rem) < int_magnitude(denom);
}

static int ldiv_matches(long numer, long denom, long quot, long rem)
{
    ldiv_t result = ldiv(numer, denom);

    if (result.quot != quot || result.rem != rem) {
        return 0;
    }
    if (result.rem != 0 && ((result.rem < 0) != (numer < 0))) {
        return 0;
    }
    return long_magnitude(result.rem) < long_magnitude(denom);
}

static int lldiv_matches(long long numer, long long denom, long long quot,
                         long long rem)
{
    lldiv_t result = lldiv(numer, denom);

    if (result.quot != quot || result.rem != rem) {
        return 0;
    }
    if (result.rem != 0 && ((result.rem < 0) != (numer < 0))) {
        return 0;
    }
    return long_long_magnitude(result.rem) < long_long_magnitude(denom);
}

int main(int argc, char **argv, char **envp)
{
    static const char ok[] = "bsearch-ok\n";
    int values[] = {1, 3, 5, 7, 9};
    int duplicates[] = {1, 2, 2, 2, 3};
    struct record records[] = {
        {2, {1, 2, 3}},
        {4, {4, 5, 6}},
        {8, {7, 8, 9}},
    };
    int sorted[] = {-3, -1, 0, 4, 8};
    int reverse[] = {9, 7, 5, 3, 1, -1};
    int sort_duplicates[] = {4, 2, 4, 1, 2, 4, 1};
    struct {
        unsigned char before;
        struct triple values[5];
        unsigned char after;
    } box = {0xa5,
             {{3, 30, 31}, {1, 10, 11}, {2, 20, 21}, {1, 12, 13}, {0, 1, 2}},
             0x5a};
    int key;
    int *found;
    struct record *record;
    int numer;
    int denom;
    long lnumer;
    long ldenom;
    long long llnumer;
    long long lldenom;

    (void)argc;
    (void)argv;
    (void)envp;

    errno = ERANGE;
    compare_calls = 0;
    key = 1;
    if (bsearch(&key, values, 0, sizeof(values[0]), counting_compare) !=
            (void *)0 ||
        compare_calls != 0 || errno != ERANGE) {
        return 1;
    }

    key = 1;
    found = (int *)bsearch(&key, values, 5, sizeof(values[0]), compare_int);
    if (found != &values[0] || errno != ERANGE) {
        return 2;
    }

    key = 5;
    found = (int *)bsearch(&key, values, 5, sizeof(values[0]), compare_int);
    if (found != &values[2] || errno != ERANGE) {
        return 3;
    }

    key = 9;
    found = (int *)bsearch(&key, values, 5, sizeof(values[0]), compare_int);
    if (found != &values[4] || errno != ERANGE) {
        return 4;
    }

    key = 4;
    if (bsearch(&key, values, 5, sizeof(values[0]), compare_int) != (void *)0 ||
        errno != ERANGE) {
        return 5;
    }

    key = 2;
    found =
        (int *)bsearch(&key, duplicates, 5, sizeof(duplicates[0]), compare_int);
    if (found == (void *)0 || *found != 2 || found < &duplicates[1] ||
        found > &duplicates[3] || errno != ERANGE) {
        return 6;
    }

    key = 4;
    record = (struct record *)bsearch(&key, records, 3, sizeof(records[0]),
                                      compare_record);
    if (record != &records[1] || record->payload[0] != 4 || errno != ERANGE) {
        return 7;
    }

    compare_calls = 0;
    qsort(sorted, 0, sizeof(sorted[0]), counting_compare);
    if (compare_calls != 0 || sorted[0] != -3 || errno != ERANGE) {
        return 8;
    }

    compare_calls = 0;
    qsort(sorted, 1, sizeof(sorted[0]), counting_compare);
    if (compare_calls != 0 || sorted[0] != -3 || errno != ERANGE) {
        return 9;
    }

    qsort(sorted, sizeof(sorted) / sizeof(sorted[0]), sizeof(sorted[0]),
          compare_int);
    if (!ints_sorted(sorted, sizeof(sorted) / sizeof(sorted[0])) ||
        errno != ERANGE) {
        return 10;
    }

    qsort(reverse, sizeof(reverse) / sizeof(reverse[0]), sizeof(reverse[0]),
          compare_int);
    if (!ints_sorted(reverse, sizeof(reverse) / sizeof(reverse[0])) ||
        reverse[0] != -1 || reverse[5] != 9 || errno != ERANGE) {
        return 11;
    }

    qsort(sort_duplicates,
          sizeof(sort_duplicates) / sizeof(sort_duplicates[0]),
          sizeof(sort_duplicates[0]), compare_int);
    if (!ints_sorted(sort_duplicates,
                     sizeof(sort_duplicates) / sizeof(sort_duplicates[0])) ||
        sort_duplicates[0] != 1 || sort_duplicates[1] != 1 ||
        sort_duplicates[2] != 2 || sort_duplicates[3] != 2 ||
        sort_duplicates[4] != 4 || sort_duplicates[5] != 4 ||
        sort_duplicates[6] != 4 || errno != ERANGE) {
        return 12;
    }

    qsort(box.values, sizeof(box.values) / sizeof(box.values[0]),
          sizeof(box.values[0]), compare_triple);
    if (box.before != 0xa5 || box.after != 0x5a ||
        !triples_valid(box.values, sizeof(box.values) / sizeof(box.values[0])) ||
        errno != ERANGE) {
        return 13;
    }

    if (abs(0) != 0 || abs(1) != 1 || abs(-1) != 1 ||
        abs(__INT_MAX__) != __INT_MAX__ || abs(-__INT_MAX__) != __INT_MAX__ ||
        errno != ERANGE) {
        return 14;
    }

    if (labs(0L) != 0L || labs(1L) != 1L || labs(-1L) != 1L ||
        labs(__LONG_MAX__) != __LONG_MAX__ ||
        labs(-__LONG_MAX__) != __LONG_MAX__ || errno != ERANGE) {
        return 15;
    }

    if (!div_matches(7, 3, 2, 1) || !div_matches(-7, 3, -2, -1) ||
        !div_matches(7, -3, -2, 1) || !div_matches(-7, -3, 2, -1) ||
        !div_matches(0, 7, 0, 0) || errno != ERANGE) {
        return 16;
    }

    if (!div_matches(__INT_MAX__, -1, -__INT_MAX__, 0) ||
        !div_matches(-__INT_MAX__ - 1, 2, (-__INT_MAX__ - 1) / 2,
                     (-__INT_MAX__ - 1) % 2) ||
        errno != ERANGE) {
        return 17;
    }

    for (numer = -64; numer <= 64; ++numer) {
        for (denom = -16; denom <= 16; ++denom) {
            div_t result;

            if (denom == 0) {
                continue;
            }
            result = div(numer, denom);
            if (!div_matches(numer, denom, numer / denom, numer % denom) ||
                result.quot * denom + result.rem != numer || errno != ERANGE) {
                return 18;
            }
        }
    }

    if (!ldiv_matches(7L, 3L, 2L, 1L) ||
        !ldiv_matches(-7L, 3L, -2L, -1L) ||
        !ldiv_matches(7L, -3L, -2L, 1L) ||
        !ldiv_matches(-7L, -3L, 2L, -1L) ||
        !ldiv_matches(0L, 7L, 0L, 0L) || errno != ERANGE) {
        return 19;
    }

    if (!ldiv_matches(__LONG_MAX__, -1L, -__LONG_MAX__, 0L) ||
        !ldiv_matches(-__LONG_MAX__ - 1L, 2L, (-__LONG_MAX__ - 1L) / 2L,
                      (-__LONG_MAX__ - 1L) % 2L) ||
        errno != ERANGE) {
        return 20;
    }

    for (lnumer = -128; lnumer <= 128; ++lnumer) {
        for (ldenom = -24; ldenom <= 24; ++ldenom) {
            ldiv_t result;

            if (ldenom == 0) {
                continue;
            }
            result = ldiv(lnumer, ldenom);
            if (!ldiv_matches(lnumer, ldenom, lnumer / ldenom,
                              lnumer % ldenom) ||
                result.quot * ldenom + result.rem != lnumer ||
                errno != ERANGE) {
                return 21;
            }
        }
    }

    if (llabs(0LL) != 0LL || llabs(1LL) != 1LL || llabs(-1LL) != 1LL ||
        llabs(__LONG_LONG_MAX__) != __LONG_LONG_MAX__ ||
        llabs(-__LONG_LONG_MAX__) != __LONG_LONG_MAX__ || errno != ERANGE) {
        return 22;
    }

    if (!lldiv_matches(7LL, 3LL, 2LL, 1LL) ||
        !lldiv_matches(-7LL, 3LL, -2LL, -1LL) ||
        !lldiv_matches(7LL, -3LL, -2LL, 1LL) ||
        !lldiv_matches(-7LL, -3LL, 2LL, -1LL) ||
        !lldiv_matches(0LL, 7LL, 0LL, 0LL) || errno != ERANGE) {
        return 23;
    }

    if (!lldiv_matches(__LONG_LONG_MAX__, -1LL, -__LONG_LONG_MAX__, 0LL) ||
        !lldiv_matches(-__LONG_LONG_MAX__ - 1LL, 2LL,
                       (-__LONG_LONG_MAX__ - 1LL) / 2LL,
                       (-__LONG_LONG_MAX__ - 1LL) % 2LL) ||
        errno != ERANGE) {
        return 24;
    }

    for (llnumer = -256; llnumer <= 256; ++llnumer) {
        for (lldenom = -32; lldenom <= 32; ++lldenom) {
            lldiv_t result;

            if (lldenom == 0) {
                continue;
            }
            result = lldiv(llnumer, lldenom);
            if (!lldiv_matches(llnumer, lldenom, llnumer / lldenom,
                               llnumer % lldenom) ||
                result.quot * lldenom + result.rem != llnumer ||
                errno != ERANGE) {
                return 25;
            }
        }
    }

    if (mini_sys_write(1, ok, sizeof(ok) - 1) != (long)(sizeof(ok) - 1)) {
        return 26;
    }
    return 0;
}
