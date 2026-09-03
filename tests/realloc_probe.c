#include <errno.h>
#include <mini/syscall.h>
#include <stddef.h>
#include <stdlib.h>

#define SLOT_COUNT 24
#define STRESS_STEPS 1500

static unsigned long rng_state = 0x6a09e667f3bcc909UL;

static unsigned long next_random(void)
{
    rng_state ^= rng_state << 7;
    rng_state ^= rng_state >> 9;
    rng_state ^= rng_state << 8;
    return rng_state;
}

static int aligned_16(const void *ptr)
{
    return (((__UINTPTR_TYPE__)ptr) & 15UL) == 0UL;
}

static void fill_bytes(unsigned char *ptr, size_t n, unsigned char value)
{
    size_t i;

    for (i = 0; i < n; ++i) {
        ptr[i] = value;
    }
}

static int check_bytes(const unsigned char *ptr, size_t n, unsigned char value)
{
    size_t i;

    for (i = 0; i < n; ++i) {
        if (ptr[i] != value) {
            return 0;
        }
    }
    return 1;
}

int main(int argc, char **argv, char **envp)
{
    static const char ok[] = "realloc-ok\n";
    void *slots[SLOT_COUNT];
    size_t sizes[SLOT_COUNT];
    unsigned char marks[SLOT_COUNT];
    unsigned char *a;
    unsigned char *b;
    unsigned char *c;
    unsigned char *p;
    unsigned char *q;
    unsigned char *tail;
    unsigned char *moved;
    unsigned char *replacement;
    unsigned char *zeroed;
    size_t i;
    int step;

    (void)argc;
    (void)argv;
    (void)envp;

    errno = 41;
    p = (unsigned char *)realloc((void *)0, 64);
    if (p == (unsigned char *)0 || !aligned_16(p) || errno != 41) return 1;
    fill_bytes(p, 64, 0x11U);

    errno = 42;
    q = (unsigned char *)realloc(p, 24);
    if (q != p || errno != 42 || !check_bytes(q, 24, 0x11U)) return 2;
    free(q);

    a = (unsigned char *)malloc(64);
    b = (unsigned char *)malloc(64);
    c = (unsigned char *)malloc(64);
    if (a == (unsigned char *)0 || b == (unsigned char *)0 ||
        c == (unsigned char *)0) return 3;
    fill_bytes(a, 64, 0x21U);
    fill_bytes(c, 64, 0x23U);
    free(b);

    errno = 43;
    p = (unsigned char *)realloc(a, 112);
    if (p != a || errno != 43 || !check_bytes(p, 64, 0x21U) ||
        !check_bytes(c, 64, 0x23U)) return 4;

    tail = (unsigned char *)malloc(256);
    if (tail == (unsigned char *)0) return 5;
    fill_bytes(tail, 256, 0x31U);
    errno = 44;
    q = (unsigned char *)realloc(tail, 384);
    if (q != tail || errno != 44 || !check_bytes(q, 256, 0x31U)) return 6;

    moved = (unsigned char *)malloc(80);
    replacement = (unsigned char *)malloc(80);
    if (moved == (unsigned char *)0 || replacement == (unsigned char *)0) return 7;
    fill_bytes(moved, 80, 0x41U);
    fill_bytes(replacement, 80, 0x42U);
    errno = 45;
    q = (unsigned char *)realloc(moved, 320);
    if (q == (unsigned char *)0 || q == moved || errno != 45 ||
        !check_bytes(q, 80, 0x41U) ||
        !check_bytes(replacement, 80, 0x42U)) return 8;
    zeroed = (unsigned char *)malloc(80);
    if (zeroed != moved) return 9;
    free(zeroed);
    free(q);
    free(replacement);

    p = (unsigned char *)malloc(48);
    if (p == (unsigned char *)0) return 10;
    fill_bytes(p, 48, 0x51U);
    errno = 46;
    if (realloc(p, 0) != (void *)0 || errno != 46) return 11;
    q = (unsigned char *)malloc(48);
    if (q != p) return 12;
    free(q);

    p = (unsigned char *)malloc(64);
    if (p == (unsigned char *)0) return 13;
    fill_bytes(p, 64, 0x61U);
    errno = 0;
    if (realloc(p, (size_t)-1) != (void *)0 || errno != ENOMEM ||
        !check_bytes(p, 64, 0x61U)) return 14;
    free(p);

    free(c);
    free(a);
    free(tail);

    for (i = 0; i < SLOT_COUNT; ++i) {
        slots[i] = (void *)0;
        sizes[i] = 0;
        marks[i] = 0;
    }

    for (step = 0; step < STRESS_STEPS; ++step) {
        size_t index = (size_t)(next_random() % SLOT_COUNT);

        if (slots[index] == (void *)0) {
            size_t n = (size_t)(next_random() % 160UL) + 1U;
            unsigned char mark = (unsigned char)(index + 1U);
            void *fresh = malloc(n);

            if (fresh == (void *)0 || !aligned_16(fresh)) return 20;
            slots[index] = fresh;
            sizes[index] = n;
            marks[index] = mark;
            fill_bytes((unsigned char *)fresh, n, mark);
        } else if ((next_random() & 3UL) == 0UL) {
            if (!check_bytes((const unsigned char *)slots[index], sizes[index],
                             marks[index])) return 21;
            free(slots[index]);
            slots[index] = (void *)0;
            sizes[index] = 0;
        } else {
            size_t old_size = sizes[index];
            size_t new_size = (size_t)(next_random() % 224UL) + 1U;
            size_t preserved = old_size < new_size ? old_size : new_size;
            void *resized;

            if (!check_bytes((const unsigned char *)slots[index], old_size,
                             marks[index])) return 22;
            resized = realloc(slots[index], new_size);
            if (resized == (void *)0 || !aligned_16(resized) ||
                !check_bytes((const unsigned char *)resized, preserved,
                             marks[index])) return 23;
            slots[index] = resized;
            sizes[index] = new_size;
            fill_bytes((unsigned char *)resized, new_size, marks[index]);
        }

        for (i = 0; i < SLOT_COUNT; ++i) {
            if (slots[i] != (void *)0 &&
                !check_bytes((const unsigned char *)slots[i], sizes[i], marks[i])) {
                return 24;
            }
        }
    }

    for (i = 0; i < SLOT_COUNT; ++i) {
        free(slots[i]);
    }

    errno = 47;
    if (realloc((void *)0, 0) != (void *)0 || errno != 47) return 25;

    if (mini_sys_write(1, ok, sizeof(ok) - 1) != (long)(sizeof(ok) - 1)) {
        return 26;
    }
    return 0;
}
