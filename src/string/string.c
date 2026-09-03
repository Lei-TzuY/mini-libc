#include <string.h>

size_t strlen(const char *s)
{
    size_t n = 0;

    while (s[n] != '\0') {
        ++n;
    }
    return n;
}

int strcmp(const char *left, const char *right)
{
    const unsigned char *a = (const unsigned char *)left;
    const unsigned char *b = (const unsigned char *)right;

    while (*a != 0 && *a == *b) {
        ++a;
        ++b;
    }
    return (int)*a - (int)*b;
}

int strncmp(const char *left, const char *right, size_t n)
{
    const unsigned char *a = (const unsigned char *)left;
    const unsigned char *b = (const unsigned char *)right;
    size_t i;

    for (i = 0; i < n; ++i) {
        if (a[i] != b[i] || a[i] == 0) {
            return (int)a[i] - (int)b[i];
        }
    }
    return 0;
}

char *strcpy(char *restrict dest, const char *restrict src)
{
    size_t i = 0;

    do {
        dest[i] = src[i];
    } while (src[i++] != '\0');
    return dest;
}

char *strncpy(char *restrict dest, const char *restrict src, size_t n)
{
    size_t i = 0;

    while (i < n && src[i] != '\0') {
        dest[i] = src[i];
        ++i;
    }
    while (i < n) {
        dest[i] = '\0';
        ++i;
    }
    return dest;
}

char *strchr(const char *s, int c)
{
    unsigned char target = (unsigned char)c;

    for (;;) {
        if ((unsigned char)*s == target) {
            return (char *)s;
        }
        if (*s == '\0') {
            return (char *)0;
        }
        ++s;
    }
}

char *strrchr(const char *s, int c)
{
    unsigned char target = (unsigned char)c;
    const char *last = (const char *)0;

    for (;;) {
        if ((unsigned char)*s == target) {
            last = s;
        }
        if (*s == '\0') {
            return (char *)last;
        }
        ++s;
    }
}

char *strstr(const char *haystack, const char *needle)
{
    const unsigned char *first = (const unsigned char *)haystack;
    const unsigned char *pattern = (const unsigned char *)needle;

    if (*pattern == 0) {
        return (char *)haystack;
    }

    while (*first != 0) {
        const unsigned char *h = first;
        const unsigned char *n = pattern;

        while (*n != 0 && *h != 0 && *h == *n) {
            ++h;
            ++n;
        }
        if (*n == 0) {
            return (char *)first;
        }
        ++first;
    }
    return (char *)0;
}

static int byte_in_set(const char *set, unsigned char byte)
{
    while (*set != '\0') {
        if ((unsigned char)*set == byte) {
            return 1;
        }
        ++set;
    }
    return 0;
}

size_t strspn(const char *s, const char *accept)
{
    size_t n = 0;

    while (s[n] != '\0' && byte_in_set(accept, (unsigned char)s[n])) {
        ++n;
    }
    return n;
}

size_t strcspn(const char *s, const char *reject)
{
    size_t n = 0;

    while (s[n] != '\0' && !byte_in_set(reject, (unsigned char)s[n])) {
        ++n;
    }
    return n;
}

char *strpbrk(const char *s, const char *accept)
{
    while (*s != '\0') {
        if (byte_in_set(accept, (unsigned char)*s)) {
            return (char *)s;
        }
        ++s;
    }
    return (char *)0;
}

char *strcat(char *restrict dest, const char *restrict src)
{
    char *result = dest;

    while (*dest != '\0') {
        ++dest;
    }
    do {
        *dest++ = *src;
    } while (*src++ != '\0');
    return result;
}
