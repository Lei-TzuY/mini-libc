#include <stdlib.h>

void *bsearch(const void *key, const void *base, size_t nmemb, size_t size,
              int (*compar)(const void *, const void *))
{
    const unsigned char *bytes = (const unsigned char *)base;
    size_t low = 0;
    size_t high = nmemb;

    while (low < high) {
        size_t mid = low + (high - low) / 2;
        const void *element = bytes + mid * size;
        int order = compar(key, element);

        if (order < 0) {
            high = mid;
        } else if (order > 0) {
            low = mid + 1;
        } else {
            return (void *)element;
        }
    }

    return (void *)0;
}

static void swap_bytes(unsigned char *left, unsigned char *right, size_t size)
{
    size_t i;

    for (i = 0; i < size; ++i) {
        unsigned char tmp = left[i];
        left[i] = right[i];
        right[i] = tmp;
    }
}

void qsort(void *base, size_t nmemb, size_t size,
           int (*compar)(const void *, const void *))
{
    unsigned char *bytes = (unsigned char *)base;
    size_t i;

    for (i = 1; i < nmemb; ++i) {
        size_t j = i;

        while (j != 0) {
            unsigned char *left = bytes + (j - 1) * size;
            unsigned char *right = bytes + j * size;

            if (compar(left, right) <= 0) {
                break;
            }
            swap_bytes(left, right, size);
            --j;
        }
    }
}

int abs(int j)
{
    return j < 0 ? -j : j;
}

long labs(long j)
{
    return j < 0 ? -j : j;
}
