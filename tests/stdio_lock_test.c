#include <stdatomic.h>

#include "../src/internal/thread_runtime.h"

#define TEST_FUTEX_WAIT 0
#define TEST_FUTEX_WAKE 1

static struct mini_thread_tcb test_tcbs[2];
static unsigned int current_tcb;
static int wait_count;
static int wake_count;
static int release_first_on_wait;
static int protocol_error;

struct mini_thread_tcb *__mini_thread_current_tcb(void)
{
    return &test_tcbs[current_tcb];
}

long mini_test_futex(volatile int *uaddr, int op, int value,
                     const void *timeout, volatile int *uaddr2, int value3)
{
    (void)timeout;
    (void)uaddr2;
    (void)value3;

    if (value != 1) {
        protocol_error = 1;
        return 0;
    }
    if (op == TEST_FUTEX_WAIT) {
        ++wait_count;
        if (release_first_on_wait) {
            test_tcbs[0].reserved = 0U;
            atomic_store((atomic_int *)(void *)uaddr, 0);
            release_first_on_wait = 0;
        }
        return 0;
    }
    if (op == TEST_FUTEX_WAKE) {
        ++wake_count;
        return 0;
    }

    protocol_error = 1;
    return 0;
}

void __mini_stdio_lock(void);
void __mini_stdio_unlock(void);

int main(void)
{
    test_tcbs[0].self = &test_tcbs[0];
    test_tcbs[1].self = &test_tcbs[1];

    current_tcb = 0U;
    __mini_stdio_lock();
    if (test_tcbs[0].reserved != 1U || wait_count != 0 || wake_count != 0) {
        return 1;
    }
    __mini_stdio_lock();
    if (test_tcbs[0].reserved != 2U || wait_count != 0 || wake_count != 0) {
        return 2;
    }
    __mini_stdio_unlock();
    if (test_tcbs[0].reserved != 1U || wake_count != 0) {
        return 3;
    }
    __mini_stdio_unlock();
    if (test_tcbs[0].reserved != 0U || wake_count != 1) {
        return 4;
    }

    __mini_stdio_lock();
    if (test_tcbs[0].reserved != 1U) {
        return 5;
    }
    current_tcb = 1U;
    release_first_on_wait = 1;
    __mini_stdio_lock();
    if (protocol_error || wait_count != 1 || test_tcbs[0].reserved != 0U ||
        test_tcbs[1].reserved != 1U) {
        return 6;
    }
    __mini_stdio_unlock();
    if (protocol_error || test_tcbs[1].reserved != 0U || wake_count != 2) {
        return 7;
    }

    return 0;
}
