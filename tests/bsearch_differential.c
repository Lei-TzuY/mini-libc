#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

void *mini_test_bsearch(const void *key, const void *base, size_t nmemb,
                        size_t size,
                        int (*compar)(const void *, const void *));

static uint32_t rng_state = 0x62b7c41dU;

static uint32_t next_u32(void)
{
    uint32_t x = rng_state;

    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng_state = x;
    return x;
}

static int compare_int(const void *left_ptr, const void *right_ptr)
{
    int left = *(const int *)left_ptr;
    int right = *(const int *)right_ptr;

    return left < right ? -1 : left > right ? 1 : 0;
}

static int sorted_and_same_counts(const int *before, const int *after,
                                  size_t count)
{
    unsigned int before_counts[101] = {0};
    unsigned int after_counts[101] = {0};
    size_t i;

    for (i = 1; i < count; ++i) {
        if (after[i - 1] > after[i]) {
            return 0;
        }
    }

    for (i = 0; i < count; ++i) {
        ++before_counts[(unsigned int)(before[i] + 50)];
        ++after_counts[(unsigned int)(after[i] + 50)];
    }

    for (i = 0; i < 101; ++i) {
        if (before_counts[i] != after_counts[i]) {
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    int values[64];
    int unsorted[64];
    int sorted[64];
    unsigned int iter;

    for (iter = 0; iter < 10000; ++iter) {
        size_t count = next_u32() % 65U;
        size_t i;
        int cursor = (int)(next_u32() % 31U) - 15;
        int key;
        int *host_result;
        int *mini_result;

        for (i = 0; i < count; ++i) {
            cursor += 1 + (int)(next_u32() % 4U);
            values[i] = cursor;
        }

        if (count != 0 && (next_u32() & 1U) != 0) {
            key = values[next_u32() % count];
        } else {
            key = (int)(next_u32() % 240U) - 60;
        }

        host_result = bsearch(&key, values, count, sizeof(values[0]), compare_int);
        mini_result = mini_test_bsearch(&key, values, count, sizeof(values[0]),
                                        compare_int);

        if ((host_result == (void *)0) != (mini_result == (void *)0)) {
            return 1;
        }
        if (host_result != (void *)0 &&
            host_result - values != mini_result - values) {
            return 2;
        }

        for (i = 0; i < count; ++i) {
            unsorted[i] = (int)(next_u32() % 101U) - 50;
            sorted[i] = unsorted[i];
        }
        qsort(sorted, count, sizeof(sorted[0]), compare_int);
        if (!sorted_and_same_counts(unsorted, sorted, count)) {
            return 3;
        }
    }

    return 0;
}
