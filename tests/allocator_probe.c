#include <errno.h>
#include <mini/syscall.h>
#include <stddef.h>
#include <stdlib.h>

#define SLOT_COUNT 32
#define STRESS_STEPS 2000

static unsigned long rng_state = 0x9e3779b97f4a7c15UL;

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
    static const char message[] = "allocator-ok\n";
    void *slots[SLOT_COUNT];
    size_t sizes[SLOT_COUNT];
    unsigned char marks[SLOT_COUNT];
    unsigned char *a;
    unsigned char *b;
    unsigned char *c;
    unsigned char *d;
    unsigned char *e;
    long before_break;
    long after_break;
    size_t i;
    int step;

    (void)argc;
    (void)argv;
    (void)envp;

    errno = 7;
    if (malloc(0) != (void *)0 || errno != 7) return 1;
    free((void *)0);
    if (errno != 7) return 2;

    before_break = mini_sys_brk((void *)0);
    if (before_break <= 0) return 3;
    if (mini_sys_brk((void *)1) != before_break ||
        mini_sys_brk((void *)0) != before_break) return 24;

    a = (unsigned char *)malloc(1);
    b = (unsigned char *)malloc(31);
    c = (unsigned char *)malloc(48);
    if (a == (unsigned char *)0 || b == (unsigned char *)0 ||
        c == (unsigned char *)0) return 4;
    if (errno != 7) return 25;
    if (!aligned_16(a) || !aligned_16(b) || !aligned_16(c)) return 5;
    if (a == b || a == c || b == c) return 6;

    fill_bytes(a, 1, 0xa1U);
    fill_bytes(b, 31, 0xb2U);
    fill_bytes(c, 48, 0xc3U);
    if (!check_bytes(a, 1, 0xa1U) || !check_bytes(b, 31, 0xb2U) ||
        !check_bytes(c, 48, 0xc3U)) return 7;

    after_break = mini_sys_brk((void *)0);
    if (after_break <= before_break) return 8;

    free(b);
    errno = 7;
    d = (unsigned char *)malloc(16);
    if (d != b || errno != 7) return 9;
    fill_bytes(d, 16, 0xd4U);
    if (!check_bytes(a, 1, 0xa1U) || !check_bytes(c, 48, 0xc3U)) return 10;

    /* Free adjacent blocks and require first-fit coalescing to satisfy a larger request. */
    free(a);
    free(d);
    e = (unsigned char *)malloc(32);
    if (e != a || !aligned_16(e)) return 11;
    fill_bytes(e, 32, 0xe5U);
    if (!check_bytes(c, 48, 0xc3U)) return 12;

    errno = 7;
    if (malloc((size_t)-1) != (void *)0 || errno != ENOMEM) return 13;

    for (i = 0; i < SLOT_COUNT; ++i) {
        slots[i] = (void *)0;
        sizes[i] = 0;
        marks[i] = 0;
    }

    for (step = 0; step < STRESS_STEPS; ++step) {
        size_t index = (size_t)(next_random() % SLOT_COUNT);

        if (slots[index] != (void *)0 && (next_random() & 1UL) != 0UL) {
            if (!check_bytes((const unsigned char *)slots[index], sizes[index],
                             marks[index])) return 20;
            free(slots[index]);
            slots[index] = (void *)0;
            sizes[index] = 0;
            continue;
        }

        if (slots[index] == (void *)0) {
            size_t n = (size_t)(next_random() % 192UL) + 1U;
            unsigned char mark = (unsigned char)(index + 1U);
            void *ptr = malloc(n);

            if (ptr == (void *)0 || !aligned_16(ptr)) return 21;
            slots[index] = ptr;
            sizes[index] = n;
            marks[index] = mark;
            fill_bytes((unsigned char *)ptr, n, mark);
        }

        for (i = 0; i < SLOT_COUNT; ++i) {
            if (slots[i] != (void *)0 &&
                !check_bytes((const unsigned char *)slots[i], sizes[i], marks[i])) {
                return 22;
            }
        }
    }

    for (i = 0; i < SLOT_COUNT; ++i) {
        if (slots[i] != (void *)0) {
            free(slots[i]);
        }
    }
    free(c);
    free(e);

    if (mini_sys_write(1, message, sizeof(message) - 1) !=
        (long)(sizeof(message) - 1)) return 23;
    return 0;
}
