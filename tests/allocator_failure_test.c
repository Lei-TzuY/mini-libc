#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define TEST_ENOMEM 12
#define ARENA_SIZE 512
#define TEST_FUTEX_WAIT 0
#define TEST_FUTEX_WAKE 1
#define TEST_RAW_EAGAIN (-11L)

_Alignas(16) static unsigned char arena[ARENA_SIZE];
static uintptr_t current_break = (uintptr_t)arena;
static unsigned int futex_wait_calls;
static unsigned int futex_wake_calls;

void *mini_test_malloc(size_t size);
void *mini_test_realloc(void *ptr, size_t size);
void mini_test_free(void *ptr);
int *__mini_errno_location(void);

long mini_sys_futex(volatile int *uaddr, int op, int value,
                    const void *timeout, volatile int *uaddr2, int value3)
{
    (void)uaddr;
    (void)value;
    (void)timeout;
    (void)uaddr2;
    (void)value3;

    if (op == TEST_FUTEX_WAIT) {
        ++futex_wait_calls;
        return TEST_RAW_EAGAIN;
    }
    if (op == TEST_FUTEX_WAKE) {
        ++futex_wake_calls;
        return 0L;
    }
    return -1L;
}

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

int main(void)
{
    int *mini_errno = __mini_errno_location();
    unsigned char *a;
    unsigned char *blocker;
    unsigned char *resized;
    void *reuse;

    *mini_errno = 7;
    a = (unsigned char *)mini_test_malloc(128);
    blocker = (unsigned char *)mini_test_malloc(64);
    if (a == NULL || blocker == NULL || ((uintptr_t)a & 15U) != 0U ||
        ((uintptr_t)blocker & 15U) != 0U || *mini_errno != 7 ||
        futex_wait_calls != 0U || futex_wake_calls < 2U) {
        return 1;
    }
    fill_bytes(a, 128, 0xa1U);
    fill_bytes(blocker, 64, 0xb2U);

    *mini_errno = 7;
    resized = (unsigned char *)mini_test_realloc(a, 400);
    if (resized != NULL || *mini_errno != TEST_ENOMEM ||
        !check_bytes(a, 128, 0xa1U) || !check_bytes(blocker, 64, 0xb2U)) {
        return 2;
    }

    mini_test_free(blocker);
    *mini_errno = 7;
    resized = (unsigned char *)mini_test_realloc(a, 176);
    if (resized != a || *mini_errno != 7 ||
        !check_bytes(resized, 128, 0xa1U)) {
        return 3;
    }

    *mini_errno = 7;
    resized = (unsigned char *)mini_test_realloc(a, 240);
    if (resized != a || *mini_errno != 7 ||
        !check_bytes(resized, 128, 0xa1U)) {
        return 4;
    }

    *mini_errno = 7;
    resized = (unsigned char *)mini_test_realloc(a, 600);
    if (resized != NULL || *mini_errno != TEST_ENOMEM ||
        !check_bytes(a, 128, 0xa1U)) {
        return 5;
    }

    *mini_errno = 19;
    if (mini_test_realloc(a, 0) != NULL || *mini_errno != 19) {
        return 6;
    }
    reuse = mini_test_malloc(96);
    if (reuse != a) {
        return 7;
    }

    mini_test_free(reuse);
    if (futex_wait_calls != 0U || futex_wake_calls == 0U) {
        return 8;
    }
    puts("allocator failure-path test passed");
    return 0;
}
