#include <stdatomic.h>
#include <threads.h>
#include <mini/syscall.h>

#define WORKER_COUNT 4
#define ITERATIONS 20000

static atomic_int counter = ATOMIC_VAR_INIT(0);
static atomic_int ready = ATOMIC_VAR_INIT(0);
static int published_value;
static int pointer_values[16];
static int * _Atomic pointer_cursor = pointer_values + 2;
static atomic_flag gate = ATOMIC_FLAG_INIT;

static int counter_worker(void *unused)
{
    int i;

    (void)unused;
    for (i = 0; i < ITERATIONS; ++i) {
        (void)atomic_fetch_add_explicit(&counter, 1, memory_order_relaxed);
    }
    return 0;
}

static int publisher(void *unused)
{
    (void)unused;
    published_value = 0x12345;
    atomic_store_explicit(&ready, 1, memory_order_release);
    return 19;
}

int main(void)
{
    atomic_int value = ATOMIC_VAR_INIT(3);
    atomic_uint bits = ATOMIC_VAR_INIT(3U);
    _Atomic double floating = 1.5;
    thrd_t threads[WORKER_COUNT];
    thrd_t pub;
    int expected;
    double expected_double;
    int result;
    int i;
    int *expected_pointer;
    static const char marker[] = "atomics-ok\n";

    _Static_assert(sizeof(atomic_int) == sizeof(int),
                   "atomic int representation");
    _Static_assert(sizeof(atomic_long) == sizeof(long),
                   "atomic long representation");
    _Static_assert(ATOMIC_BOOL_LOCK_FREE == 2, "bool lock-free");
    _Static_assert(ATOMIC_INT_LOCK_FREE == 2, "int lock-free");
    _Static_assert(ATOMIC_LONG_LOCK_FREE == 2, "long lock-free");
    _Static_assert(ATOMIC_POINTER_LOCK_FREE == 2, "pointer lock-free");

    atomic_init(&value, 5);
    if (atomic_load(&value) != 5) {
        return 1;
    }
    atomic_store_explicit(&value, 7, memory_order_relaxed);
    if (atomic_load_explicit(&value, memory_order_relaxed) != 7) {
        return 2;
    }
    if (atomic_exchange_explicit(&value, 9, memory_order_acq_rel) != 7 ||
        atomic_load(&value) != 9) {
        return 3;
    }
    if (atomic_fetch_add(&value, 4) != 9 || atomic_load(&value) != 13 ||
        atomic_fetch_sub_explicit(&value, 3, memory_order_relaxed) != 13 ||
        atomic_load(&value) != 10) {
        return 4;
    }

    if (atomic_fetch_or(&bits, 8U) != 3U || atomic_load(&bits) != 11U ||
        atomic_fetch_and(&bits, 14U) != 11U || atomic_load(&bits) != 10U ||
        atomic_fetch_xor_explicit(&bits, 6U, memory_order_relaxed) != 10U ||
        atomic_load(&bits) != 12U) {
        return 5;
    }

    expected = 10;
    if (!atomic_compare_exchange_strong_explicit(&value, &expected, 21,
                                                  memory_order_acq_rel,
                                                  memory_order_acquire) ||
        atomic_load(&value) != 21 || expected != 10) {
        return 6;
    }
    expected = 99;
    if (atomic_compare_exchange_weak(&value, &expected, 33) ||
        expected != 21 || atomic_load(&value) != 21) {
        return 7;
    }

    if (!atomic_is_lock_free(&value) || !atomic_is_lock_free(&pointer_cursor)) {
        return 8;
    }
    if (atomic_load(&pointer_cursor) != pointer_values + 2 ||
        atomic_fetch_add(&pointer_cursor, 3) != pointer_values + 2 ||
        atomic_load(&pointer_cursor) != pointer_values + 5 ||
        atomic_fetch_sub_explicit(&pointer_cursor, 2, memory_order_relaxed) !=
            pointer_values + 5 ||
        atomic_load(&pointer_cursor) != pointer_values + 3) {
        return 9;
    }
    expected_pointer = pointer_values + 3;
    if (!atomic_compare_exchange_strong(&pointer_cursor, &expected_pointer,
                                         pointer_values + 8) ||
        atomic_load(&pointer_cursor) != pointer_values + 8) {
        return 10;
    }

    if (atomic_load(&floating) != 1.5 ||
        atomic_exchange(&floating, 2.5) != 1.5 ||
        atomic_load(&floating) != 2.5) {
        return 11;
    }
    expected_double = 2.5;
    if (!atomic_compare_exchange_strong(&floating, &expected_double, 3.5) ||
        atomic_load(&floating) != 3.5) {
        return 12;
    }

    if (atomic_flag_test_and_set_explicit(&gate, memory_order_acquire)) {
        return 13;
    }
    if (!atomic_flag_test_and_set(&gate)) {
        return 14;
    }
    atomic_flag_clear_explicit(&gate, memory_order_release);
    if (atomic_flag_test_and_set(&gate)) {
        return 15;
    }
    atomic_flag_clear(&gate);

    atomic_thread_fence(memory_order_seq_cst);
    atomic_signal_fence(memory_order_acq_rel);
    if (kill_dependency(17) != 17) {
        return 16;
    }

    atomic_store(&counter, 0);
    for (i = 0; i < WORKER_COUNT; ++i) {
        if (thrd_create(&threads[i], counter_worker, (void *)0) != thrd_success) {
            return 17;
        }
    }
    for (i = 0; i < WORKER_COUNT; ++i) {
        if (thrd_join(threads[i], &result) != thrd_success || result != 0) {
            return 18;
        }
    }
    if (atomic_load_explicit(&counter, memory_order_relaxed) !=
        WORKER_COUNT * ITERATIONS) {
        return 19;
    }

    published_value = 0;
    atomic_store_explicit(&ready, 0, memory_order_relaxed);
    if (thrd_create(&pub, publisher, (void *)0) != thrd_success) {
        return 20;
    }
    while (!atomic_load_explicit(&ready, memory_order_acquire)) {
        thrd_yield();
    }
    if (published_value != 0x12345 ||
        thrd_join(pub, &result) != thrd_success || result != 19) {
        return 21;
    }

    if (mini_sys_write(1, marker, sizeof(marker) - 1) !=
        (long)(sizeof(marker) - 1)) {
        return 22;
    }
    return 0;
}
