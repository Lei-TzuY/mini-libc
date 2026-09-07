#include <stdatomic.h>
#include <threads.h>
#include <mini/syscall.h>

#define WORKERS 3
#define LOOPS 12000

static atomic_int counter = ATOMIC_VAR_INIT(0);
static atomic_int published = ATOMIC_VAR_INIT(0);
static int payload;
static int values[12];
static int * _Atomic cursor = values + 1;

static int incrementer(void *unused)
{
    int i;

    (void)unused;
    for (i = 0; i < LOOPS; ++i) {
        (void)atomic_fetch_add_explicit(&counter, 1, memory_order_relaxed);
    }
    return 0;
}

static int producer(void *unused)
{
    (void)unused;
    payload = 77;
    atomic_store_explicit(&published, 1, memory_order_release);
    return 23;
}

int main(void)
{
    atomic_int value = ATOMIC_VAR_INIT(4);
    _Atomic double floating = 1.25;
    atomic_flag flag = ATOMIC_FLAG_INIT;
    thrd_t workers[WORKERS];
    thrd_t producer_thread;
    int expected;
    double expected_double;
    int *expected_pointer;
    int result;
    int i;
    static const char marker[] = "tiny-atomics-ok\n";

    _Static_assert(sizeof(atomic_int) == sizeof(int), "atomic int size");
    _Static_assert(ATOMIC_INT_LOCK_FREE == 2, "atomic int lock-free");
    _Static_assert(ATOMIC_POINTER_LOCK_FREE == 2, "pointer lock-free");

    atomic_store_explicit(&value, 8, memory_order_relaxed);
    if (atomic_exchange(&value, 10) != 8 ||
        atomic_fetch_add(&value, 5) != 10 || atomic_load(&value) != 15) {
        return 1;
    }
    expected = 15;
    if (!atomic_compare_exchange_strong_explicit(&value, &expected, 31, memory_order_acq_rel, memory_order_acquire) || atomic_load(&value) != 31) {
        return 2;
    }
    expected = 4;
    if (atomic_compare_exchange_weak(&value, &expected, 99) || expected != 31) {
        return 3;
    }

    if (atomic_load(&cursor) != values + 1 ||
        atomic_exchange(&cursor, values + 4) != values + 1 ||
        atomic_load(&cursor) != values + 4) {
        return 4;
    }
    expected_pointer = values + 4;
    if (!atomic_compare_exchange_strong(&cursor, &expected_pointer, values + 7) || atomic_load(&cursor) != values + 7) {
        return 5;
    }

    if (atomic_exchange(&floating, 2.5) != 1.25) {
        return 6;
    }
    expected_double = 2.5;
    if (!atomic_compare_exchange_strong(&floating, &expected_double, 4.5) || atomic_load(&floating) != 4.5) {
        return 7;
    }

    if (atomic_flag_test_and_set_explicit(&flag, memory_order_acquire)) {
        return 8;
    }
    atomic_flag_clear_explicit(&flag, memory_order_release);
    if (atomic_flag_test_and_set(&flag)) {
        return 9;
    }
    atomic_flag_clear(&flag);
    atomic_thread_fence(memory_order_seq_cst);
    atomic_signal_fence(memory_order_acq_rel);

    atomic_store(&counter, 0);
    for (i = 0; i < WORKERS; ++i) {
        if (thrd_create(&workers[i], incrementer, (void *)0) != thrd_success) {
            return 10;
        }
    }
    for (i = 0; i < WORKERS; ++i) {
        if (thrd_join(workers[i], &result) != thrd_success || result != 0) {
            return 11;
        }
    }
    if (atomic_load(&counter) != WORKERS * LOOPS) {
        return 12;
    }

    payload = 0;
    atomic_store_explicit(&published, 0, memory_order_relaxed);
    if (thrd_create(&producer_thread, producer, (void *)0) != thrd_success) {
        return 13;
    }
    while (!atomic_load_explicit(&published, memory_order_acquire)) {
        thrd_yield();
    }
    if (payload != 77 ||
        thrd_join(producer_thread, &result) != thrd_success || result != 23) {
        return 14;
    }

    if (mini_sys_write(1, marker, sizeof(marker) - 1) !=
        (long)(sizeof(marker) - 1)) {
        return 15;
    }
    return 0;
}
