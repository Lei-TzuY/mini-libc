#include <errno.h>
#include <mini/syscall.h>
#include <stddef.h>
#include <string.h>

#define BUF_SIZE 64
#define RANDOM_CASES 2000

static unsigned long rng_state = 0x9e3779b97f4a7c15UL;

static unsigned long next_random(void)
{
    rng_state ^= rng_state << 7;
    rng_state ^= rng_state >> 9;
    rng_state ^= rng_state << 8;
    return rng_state;
}

static void fill_random(unsigned char *buf, size_t n)
{
    size_t i;
    for (i = 0; i < n; ++i) {
        buf[i] = (unsigned char)next_random();
    }
}

static int equal_bytes(const unsigned char *a, const unsigned char *b, size_t n)
{
    size_t i;
    for (i = 0; i < n; ++i) {
        if (a[i] != b[i]) return 0;
    }
    return 1;
}

static void reference_move(unsigned char *dest, const unsigned char *src, size_t n)
{
    unsigned char temp[BUF_SIZE];
    size_t i;
    for (i = 0; i < n; ++i) temp[i] = src[i];
    for (i = 0; i < n; ++i) dest[i] = temp[i];
}

static int sign_of(int value)
{
    return (value > 0) - (value < 0);
}

static int reference_compare(const unsigned char *a, const unsigned char *b, size_t n)
{
    size_t i;
    for (i = 0; i < n; ++i) {
        if (a[i] != b[i]) return (int)a[i] - (int)b[i];
    }
    return 0;
}

static const void *reference_find(const unsigned char *s, int c, size_t n)
{
    unsigned char target = (unsigned char)c;
    size_t i;

    for (i = 0; i < n; ++i) {
        if (s[i] == target) return s + i;
    }
    return (const void *)0;
}

static int deterministic_cases(void)
{
    unsigned char a[8] = {0,1,2,3,4,5,6,7};
    unsigned char b[8] = {9,9,9,9,9,9,9,9};
    unsigned char overlap[8] = {0,1,2,3,4,5,6,7};
    unsigned char hi[1] = {0x80};
    unsigned char lo[1] = {0x7f};
    unsigned char search[7] = {'a',0,'b',0xff,'a','z',0x80};

    if (memcpy(b, a, 0) != b || b[0] != 9) return 1;
    if (memcpy(b, a, 8) != b || !equal_bytes(a, b, 8)) return 2;
    if (memset(b, 0x1ff, 3) != b || b[0] != 0xff || b[2] != 0xff) return 3;
    if (memcmp(a, a, 8) != 0 || memcmp(hi, lo, 1) <= 0) return 4;
    if (memmove(overlap + 2, overlap, 6) != overlap + 2) return 5;
    {
        unsigned char expected[8] = {0,1,0,1,2,3,4,5};
        if (!equal_bytes(overlap, expected, 8)) return 6;
    }
    {
        unsigned char reverse[8] = {0,1,2,3,4,5,6,7};
        unsigned char expected[8] = {2,3,4,5,6,7,6,7};
        memmove(reverse, reverse + 2, 6);
        if (!equal_bytes(reverse, expected, 8)) return 7;
    }

    errno = ERANGE;
    if (memchr(search, 'a', 0) != (void *)0 || errno != ERANGE) return 8;
    if (memchr(search, 'a', sizeof(search)) != search ||
        memchr(search, 0, sizeof(search)) != search + 1 ||
        memchr(search, 0x1ff, sizeof(search)) != search + 3 ||
        memchr(search, -1, sizeof(search)) != search + 3 ||
        memchr(search, 'z', 5) != (void *)0 ||
        memchr(search, 0x180, sizeof(search)) != search + 6 || errno != ERANGE) return 9;
    return 0;
}

static int randomized_cases(void)
{
    unsigned char source[BUF_SIZE];
    unsigned char actual[BUF_SIZE];
    unsigned char expected[BUF_SIZE];
    int case_no;

    for (case_no = 0; case_no < RANDOM_CASES; ++case_no) {
        size_t i;
        size_t n;
        size_t src_off;
        size_t dst_off;
        int c;

        fill_random(source, BUF_SIZE);
        fill_random(actual, BUF_SIZE);
        for (i = 0; i < BUF_SIZE; ++i) expected[i] = actual[i];
        n = (size_t)(next_random() % (BUF_SIZE + 1));
        src_off = (size_t)(next_random() % (BUF_SIZE - n + 1));
        dst_off = (size_t)(next_random() % (BUF_SIZE - n + 1));
        memcpy(actual + dst_off, source + src_off, n);
        for (i = 0; i < n; ++i) expected[dst_off + i] = source[src_off + i];
        if (!equal_bytes(actual, expected, BUF_SIZE)) return 10;

        fill_random(actual, BUF_SIZE);
        for (i = 0; i < BUF_SIZE; ++i) expected[i] = actual[i];
        n = (size_t)(next_random() % (BUF_SIZE + 1));
        dst_off = (size_t)(next_random() % (BUF_SIZE - n + 1));
        c = (int)next_random();
        memset(actual + dst_off, c, n);
        for (i = 0; i < n; ++i) expected[dst_off + i] = (unsigned char)c;
        if (!equal_bytes(actual, expected, BUF_SIZE)) return 11;

        fill_random(actual, BUF_SIZE);
        for (i = 0; i < BUF_SIZE; ++i) expected[i] = actual[i];
        n = (size_t)(next_random() % (BUF_SIZE + 1));
        src_off = (size_t)(next_random() % (BUF_SIZE - n + 1));
        dst_off = (size_t)(next_random() % (BUF_SIZE - n + 1));
        memmove(actual + dst_off, actual + src_off, n);
        reference_move(expected + dst_off, expected + src_off, n);
        if (!equal_bytes(actual, expected, BUF_SIZE)) return 12;

        fill_random(actual, BUF_SIZE);
        fill_random(expected, BUF_SIZE);
        n = (size_t)(next_random() % (BUF_SIZE + 1));
        if (sign_of(memcmp(actual, expected, n)) !=
            sign_of(reference_compare(actual, expected, n))) return 13;

        fill_random(source, BUF_SIZE);
        n = (size_t)(next_random() % (BUF_SIZE + 1));
        c = (int)(next_random() & 0x3ffUL) - 512;
        if (memchr(source, c, n) != reference_find(source, c, n)) return 14;
    }
    return 0;
}

int main(int argc, char **argv, char **envp)
{
    static const char ok[] = "memory-ok\n";
    int result;
    (void)argc;
    (void)argv;
    (void)envp;

    result = deterministic_cases();
    if (result != 0) return result;
    result = randomized_cases();
    if (result != 0) return result;
    return mini_sys_write(1, ok, sizeof(ok) - 1) == (long)(sizeof(ok) - 1) ? 0 : 20;
}
