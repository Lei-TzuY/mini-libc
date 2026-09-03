#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define TEST_ENOMEM 12
#define ARENA_SIZE 512

_Alignas(16) static unsigned char arena[ARENA_SIZE];
static uintptr_t current_break = (uintptr_t)arena;

void *mini_test_malloc(size_t size);
void mini_test_free(void *ptr);
int *__mini_errno_location(void);

long mini_test_brk(void *addr)
{
    uintptr_t requested;
    uintptr_t start = (uintptr_t)arena;
    uintptr_t limit = start + ARENA_SIZE;

    if (addr == NULL) {
        return (long)current_break;
    }

    requested = (uintptr_t)addr;
    if (requested >= start && requested <= limit) {
        current_break = requested;
    }
    return (long)current_break;
}

int main(void)
{
    int *mini_errno = __mini_errno_location();
    void *a;
    void *b;

    *mini_errno = 7;
    a = mini_test_malloc(128);
    if (a == NULL || ((uintptr_t)a & 15U) != 0U || *mini_errno != 7) {
        return 1;
    }

    *mini_errno = 7;
    b = mini_test_malloc(400);
    if (b != NULL || *mini_errno != TEST_ENOMEM) {
        return 2;
    }

    mini_test_free(a);
    *mini_errno = 7;
    b = mini_test_malloc(96);
    if (b != a || *mini_errno != 7) {
        return 3;
    }

    mini_test_free(b);
    puts("allocator failure-path test passed");
    return 0;
}
