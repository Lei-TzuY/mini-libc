#include <errno.h>
#include <mini/syscall.h>
#include <stddef.h>
#include <stdlib.h>

struct record {
    int key;
    unsigned char payload[3];
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

static int compare_calls;

static int counting_compare(const void *key_ptr, const void *element_ptr)
{
    (void)key_ptr;
    (void)element_ptr;
    ++compare_calls;
    return 0;
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

    if (mini_sys_write(1, ok, sizeof(ok) - 1) != (long)(sizeof(ok) - 1)) {
        return 8;
    }
    return 0;
}
