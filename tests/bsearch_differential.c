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

static int compare_int(const void *key_ptr, const void *element_ptr)
{
    int key = *(const int *)key_ptr;
    int element = *(const int *)element_ptr;

    return key < element ? -1 : key > element ? 1 : 0;
}

int main(void)
{
    int values[64];
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
    }

    return 0;
}
