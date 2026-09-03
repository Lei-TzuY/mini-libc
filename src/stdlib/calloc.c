#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

void *calloc(size_t nmemb, size_t size)
{
    size_t total;
    void *ptr;

    if (nmemb == 0 || size == 0) {
        return (void *)0;
    }
    if (nmemb > (size_t)-1 / size) {
        errno = ENOMEM;
        return (void *)0;
    }

    total = nmemb * size;
    ptr = malloc(total);
    if (ptr == (void *)0) {
        return (void *)0;
    }

    memset(ptr, 0, total);
    return ptr;
}
