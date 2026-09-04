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

    if (mini_sys_write(1, ok, sizeof(ok) - 1) != (long)(sizeof(ok) - 1)) {
        return 16;
    }
    return 0;
}
