#include <errno.h>
#include <mini/syscall.h>
#include <threads.h>

#include "../internal/thread_runtime.h"

#define MINI_FUTEX_WAIT 0
#define MINI_FUTEX_WAKE 1

#define MINI_TSS_INDEX_BITS 5U
#define MINI_TSS_INDEX_MASK ((1U << MINI_TSS_INDEX_BITS) - 1U)
#define MINI_TSS_GENERATION_MASK 0x07ffffffU

struct mini_tss_key_slot {
    unsigned int generation;
    int active;
    tss_dtor_t destructor;
};

static struct mini_tss_key_slot mini_tss_keys[MINI_TSS_MAX_KEYS];
static volatile int mini_tss_registry_lock_word;

extern int __mini_atomic_exchange_int(volatile int *value, int replacement);

static void tss_lock(void)
{
    while (__mini_atomic_exchange_int(&mini_tss_registry_lock_word, 1) != 0) {
        (void)mini_sys_futex(&mini_tss_registry_lock_word, MINI_FUTEX_WAIT, 1,
                             (const void *)0, (volatile int *)0, 0);
    }
}

static void tss_unlock(void)
{
    if (__mini_atomic_exchange_int(&mini_tss_registry_lock_word, 0) != 0) {
        (void)mini_sys_futex(&mini_tss_registry_lock_word, MINI_FUTEX_WAKE, 1,
                             (const void *)0, (volatile int *)0, 0);
    }
}

static int decode_key(tss_t key, unsigned int *index,
                      unsigned int *generation)
{
    unsigned int raw = (unsigned int)key;
    unsigned int candidate_index = raw & MINI_TSS_INDEX_MASK;
    unsigned int candidate_generation = raw >> MINI_TSS_INDEX_BITS;

    if (candidate_index >= MINI_TSS_MAX_KEYS || candidate_generation == 0U) {
        return 0;
    }
    *index = candidate_index;
    *generation = candidate_generation;
    return 1;
}

static int key_is_active_locked(unsigned int index, unsigned int generation)
{
    return mini_tss_keys[index].active &&
           mini_tss_keys[index].generation == generation;
}

int tss_create(tss_t *key, tss_dtor_t dtor)
{
    unsigned int index;
    int saved_errno = errno;

    if (key == (tss_t *)0) {
        errno = saved_errno;
        return thrd_error;
    }

    tss_lock();
    for (index = 0U; index < MINI_TSS_MAX_KEYS; ++index) {
        if (!mini_tss_keys[index].active) {
            unsigned int generation =
                (mini_tss_keys[index].generation + 1U) &
                MINI_TSS_GENERATION_MASK;

            if (generation == 0U) {
                generation = 1U;
            }
            mini_tss_keys[index].generation = generation;
            mini_tss_keys[index].active = 1;
            mini_tss_keys[index].destructor = dtor;
            *key = (tss_t)((generation << MINI_TSS_INDEX_BITS) | index);
            tss_unlock();
            errno = saved_errno;
            return thrd_success;
        }
    }
    tss_unlock();
    errno = saved_errno;
    return thrd_nomem;
}

void tss_delete(tss_t key)
{
    struct mini_thread_tcb *tcb = __mini_thread_current_tcb();
    unsigned int index;
    unsigned int generation;
    int saved_errno = errno;

    if (!decode_key(key, &index, &generation)) {
        errno = saved_errno;
        return;
    }

    tss_lock();
    if (key_is_active_locked(index, generation)) {
        mini_tss_keys[index].active = 0;
        mini_tss_keys[index].destructor = (tss_dtor_t)0;
    }
    tss_unlock();

    if (tcb->tss_generations[index] == generation) {
        tcb->tss_values[index] = (void *)0;
        tcb->tss_generations[index] = 0U;
    }
    errno = saved_errno;
}

void *tss_get(tss_t key)
{
    struct mini_thread_tcb *tcb = __mini_thread_current_tcb();
    unsigned int index;
    unsigned int generation;
    void *value = (void *)0;
    int saved_errno = errno;

    if (!decode_key(key, &index, &generation)) {
        errno = saved_errno;
        return (void *)0;
    }

    tss_lock();
    if (key_is_active_locked(index, generation) &&
        tcb->tss_generations[index] == generation) {
        value = tcb->tss_values[index];
    }
    tss_unlock();
    errno = saved_errno;
    return value;
}

int tss_set(tss_t key, void *value)
{
    struct mini_thread_tcb *tcb = __mini_thread_current_tcb();
    unsigned int index;
    unsigned int generation;
    int valid;
    int saved_errno = errno;

    if (!decode_key(key, &index, &generation)) {
        errno = saved_errno;
        return thrd_error;
    }

    tss_lock();
    valid = key_is_active_locked(index, generation);
    tss_unlock();
    if (!valid) {
        errno = saved_errno;
        return thrd_error;
    }

    tcb->tss_generations[index] = generation;
    tcb->tss_values[index] = value;
    errno = saved_errno;
    return thrd_success;
}

void __mini_tss_run_destructors(void)
{
    struct mini_thread_tcb *tcb = __mini_thread_current_tcb();
    unsigned int pass;

    for (pass = 0U; pass < TSS_DTOR_ITERATIONS; ++pass) {
        tss_dtor_t destructors[MINI_TSS_MAX_KEYS];
        void *values[MINI_TSS_MAX_KEYS];
        unsigned int index;
        int pending = 0;

        tss_lock();
        for (index = 0U; index < MINI_TSS_MAX_KEYS; ++index) {
            destructors[index] = (tss_dtor_t)0;
            values[index] = (void *)0;
            if (mini_tss_keys[index].active &&
                mini_tss_keys[index].destructor != (tss_dtor_t)0 &&
                tcb->tss_generations[index] ==
                    mini_tss_keys[index].generation &&
                tcb->tss_values[index] != (void *)0) {
                destructors[index] = mini_tss_keys[index].destructor;
                values[index] = tcb->tss_values[index];
                tcb->tss_values[index] = (void *)0;
                pending = 1;
            }
        }
        tss_unlock();

        if (!pending) {
            return;
        }
        for (index = 0U; index < MINI_TSS_MAX_KEYS; ++index) {
            if (destructors[index] != (tss_dtor_t)0) {
                destructors[index](values[index]);
            }
        }
    }
}
