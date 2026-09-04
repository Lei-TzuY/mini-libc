#include <string.h>

typedef unsigned long mini_uintptr_t;

void *memcpy(void *restrict dest, const void *restrict src, size_t n)
{
    unsigned char *d = dest;
    const unsigned char *s = src;
    size_t i;

    for (i = 0; i < n; ++i) {
        d[i] = s[i];
    }
    return dest;
}

void *memmove(void *dest, const void *src, size_t n)
{
    unsigned char *d = dest;
    const unsigned char *s = src;
    mini_uintptr_t d_addr = (mini_uintptr_t)dest;
    mini_uintptr_t s_addr = (mini_uintptr_t)src;
    size_t i;

    if (d_addr <= s_addr) {
        for (i = 0; i < n; ++i) {
            d[i] = s[i];
        }
    } else {
        for (i = n; i != 0; --i) {
            d[i - 1] = s[i - 1];
        }
    }
    return dest;
}

void *memset(void *s, int c, size_t n)
{
    unsigned char *p = s;
    unsigned char byte = (unsigned char)c;
    size_t i;

    for (i = 0; i < n; ++i) {
        p[i] = byte;
    }
    return s;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
    const unsigned char *a = s1;
    const unsigned char *b = s2;
    size_t i;

    for (i = 0; i < n; ++i) {
        if (a[i] != b[i]) {
            return (int)a[i] - (int)b[i];
        }
    }
    return 0;
}

void *memchr(const void *s, int c, size_t n)
{
    const unsigned char *p = s;
    unsigned char target = (unsigned char)c;
    size_t i;

    for (i = 0; i < n; ++i) {
        if (p[i] == target) {
            return (void *)(p + i);
        }
    }
    return (void *)0;
}
