#include <errno.h>
#include <mini/syscall.h>
#include <threads.h>

#define TEST_TSS_KEYS 32U
#define TEST_FUTEX_WAIT 0
#define TEST_FUTEX_WAKE 1
#define TEST_RAW_EINVAL (-22L)

struct mini_thread_tcb {
    struct mini_thread_tcb *self;
    void *control;
    int errno_value;
    unsigned int reserved;
    void *tss_values[TEST_TSS_KEYS];
    unsigned int tss_generations[TEST_TSS_KEYS];
};

static struct mini_thread_tcb fake_tcbs[2];
static unsigned int current_tcb;
static unsigned int wait_calls;
static unsigned int wake_calls;
static unsigned int once_calls;
static tss_t rearm_key;
static unsigned int destructor_calls;

void __mini_tss_run_destructors(void);

struct mini_thread_tcb *__mini_thread_current_tcb(void)
{
    return &fake_tcbs[current_tcb];
}

int __mini_atomic_exchange_int(volatile int *value, int replacement)
{
    int previous = *value;

    *value = replacement;
    return previous;
}

long mini_sys_futex(volatile int *uaddr, int op, int value,
                    const void *timeout, volatile int *uaddr2, int value3)
{
    (void)timeout;
    (void)uaddr2;
    (void)value3;

    if (op == TEST_FUTEX_WAIT) {
        ++wait_calls;
        if (*uaddr == value) {
            *uaddr = 2;
        }
        return 0L;
    }
    if (op == TEST_FUTEX_WAKE) {
        ++wake_calls;
        return 1L;
    }
    return TEST_RAW_EINVAL;
}

static void once_initializer(void)
{
    ++once_calls;
}

static void rearming_destructor(void *opaque)
{
    ++destructor_calls;
    (void)tss_set(rearm_key, opaque);
}

static void counting_destructor(void *opaque)
{
    unsigned int *count = (unsigned int *)opaque;

    ++*count;
}

int main(void)
{
    static int first_value;
    static int second_value;
    static int destructor_value;
    once_flag completed = ONCE_FLAG_INIT;
    once_flag waiting = {1};
    tss_t key;
    tss_t replacement;
    tss_t keys[TEST_TSS_KEYS];
    unsigned int delete_destructor_calls = 0U;
    unsigned int i;

    call_once(&completed, once_initializer);
    call_once(&completed, once_initializer);
    if (once_calls != 1U || completed.__state != 2) {
        return 1;
    }

    call_once(&waiting, once_initializer);
    if (once_calls != 1U || waiting.__state != 2 || wait_calls == 0U) {
        return 2;
    }

    errno = EIO;
    if (tss_create((tss_t *)0, (tss_dtor_t)0) != thrd_error || errno != EIO ||
        tss_create(&key, (tss_dtor_t)0) != thrd_success || errno != EIO) {
        return 3;
    }

    current_tcb = 0U;
    if (tss_set(key, &first_value) != thrd_success ||
        tss_get(key) != &first_value || errno != EIO) {
        return 4;
    }
    current_tcb = 1U;
    if (tss_get(key) != (void *)0 ||
        tss_set(key, &second_value) != thrd_success ||
        tss_get(key) != &second_value || errno != EIO) {
        return 5;
    }

    current_tcb = 0U;
    tss_delete(key);
    if (errno != EIO || tss_set(key, &first_value) != thrd_error ||
        tss_create(&replacement, (tss_dtor_t)0) != thrd_success ||
        replacement == key || tss_get(replacement) != (void *)0) {
        return 6;
    }
    current_tcb = 1U;
    if (tss_get(replacement) != (void *)0) {
        return 7;
    }
    tss_delete(replacement);

    current_tcb = 0U;
    for (i = 0U; i < TEST_TSS_KEYS; ++i) {
        if (tss_create(&keys[i], (tss_dtor_t)0) != thrd_success) {
            return 8;
        }
    }
    if (tss_create(&replacement, (tss_dtor_t)0) != thrd_nomem || errno != EIO) {
        return 9;
    }
    for (i = 0U; i < TEST_TSS_KEYS; ++i) {
        tss_delete(keys[i]);
    }

    destructor_calls = 0U;
    if (tss_create(&rearm_key, rearming_destructor) != thrd_success ||
        tss_set(rearm_key, &destructor_value) != thrd_success) {
        return 10;
    }
    __mini_tss_run_destructors();
    if (destructor_calls != TSS_DTOR_ITERATIONS ||
        tss_get(rearm_key) != &destructor_value) {
        return 11;
    }
    if (tss_set(rearm_key, (void *)0) != thrd_success) {
        return 12;
    }
    tss_delete(rearm_key);

    if (tss_create(&key, (tss_dtor_t)0) != thrd_success ||
        tss_set(key, &first_value) != thrd_success) {
        return 13;
    }
    __mini_tss_run_destructors();
    if (tss_get(key) != &first_value) {
        return 14;
    }
    tss_delete(key);

    if (tss_create(&key, counting_destructor) != thrd_success ||
        tss_set(key, &delete_destructor_calls) != thrd_success) {
        return 15;
    }
    tss_delete(key);
    if (delete_destructor_calls != 0U || errno != EIO || wake_calls == 0U) {
        return 16;
    }

    return 0;
}
