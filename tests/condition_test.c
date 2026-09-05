#include <errno.h>
#include <mini/syscall.h>
#include <threads.h>

#define TEST_FUTEX_WAIT 0
#define TEST_FUTEX_WAKE 1
#define TEST_RAW_EINTR (-4L)
#define TEST_RAW_EAGAIN (-11L)
#define TEST_RAW_EINVAL (-22L)
#define TEST_WAKE_ALL 0x7fffffff

static unsigned int lock_calls;
static unsigned int unlock_calls;
static unsigned int wait_calls;
static unsigned int wake_calls;
static int last_wait_expected;
static int last_wake_count;
static int fail_unlock;
static int fail_lock;
static int wake_result;
static long wait_results[4];
static unsigned int wait_result_count;
static unsigned int wait_result_index;
static cnd_t *signal_on_unlock_cond;

int __mini_atomic_exchange_int(volatile int *value, int replacement)
{
    int previous = *value;

    *value = replacement;
    return previous;
}

int __mini_atomic_fetch_add_int(volatile int *value, int increment)
{
    int previous = *value;

    *value = previous + increment;
    return previous;
}

int mini_test_mtx_lock(mtx_t *mtx)
{
    ++lock_calls;
    if (fail_lock) {
        fail_lock = 0;
        return thrd_error;
    }
    mtx->__state = 1;
    return thrd_success;
}

int mini_test_mtx_unlock(mtx_t *mtx)
{
    ++unlock_calls;
    if (fail_unlock) {
        fail_unlock = 0;
        return thrd_error;
    }
    mtx->__state = 0;
    if (signal_on_unlock_cond != (cnd_t *)0) {
        (void)__mini_atomic_fetch_add_int(
            (volatile int *)&signal_on_unlock_cond->__sequence, 1);
        signal_on_unlock_cond = (cnd_t *)0;
    }
    return thrd_success;
}

long mini_test_futex(volatile int *uaddr, int op, int value,
                     const void *timeout, volatile int *uaddr2, int value3)
{
    (void)timeout;
    (void)uaddr2;
    (void)value3;

    if (op == TEST_FUTEX_WAIT) {
        ++wait_calls;
        last_wait_expected = value;
        if (wait_result_index < wait_result_count) {
            return wait_results[wait_result_index++];
        }
        return *uaddr == value ? 0L : TEST_RAW_EAGAIN;
    }
    if (op == TEST_FUTEX_WAKE) {
        ++wake_calls;
        last_wake_count = value;
        return (long)wake_result;
    }
    return TEST_RAW_EINVAL;
}

static void reset_calls(void)
{
    lock_calls = 0U;
    unlock_calls = 0U;
    wait_calls = 0U;
    wake_calls = 0U;
    last_wait_expected = -1;
    last_wake_count = -1;
    fail_unlock = 0;
    fail_lock = 0;
    wake_result = 0;
    wait_result_count = 0U;
    wait_result_index = 0U;
    signal_on_unlock_cond = (cnd_t *)0;
}

int main(void)
{
    cnd_t cond;
    mtx_t mutex;

    errno = EIO;
    reset_calls();
    mutex.__state = 1;
    if (cnd_init(&cond) != thrd_success || cond.__sequence != 0 ||
        errno != EIO ||
        cnd_signal(&cond) != thrd_success || cond.__sequence != 1 ||
        wake_calls != 1U || last_wake_count != 1 || errno != EIO ||
        cnd_broadcast(&cond) != thrd_success || cond.__sequence != 2 ||
        wake_calls != 2U || last_wake_count != TEST_WAKE_ALL || errno != EIO) {
        return 1;
    }

    reset_calls();
    (void)cnd_init(&cond);
    mutex.__state = 1;
    signal_on_unlock_cond = &cond;
    errno = EIO;
    if (cnd_wait(&cond, &mutex) != thrd_success || errno != EIO ||
        cond.__sequence != 1 || unlock_calls != 1U || lock_calls != 1U ||
        wait_calls != 1U || last_wait_expected != 0 || mutex.__state != 1) {
        return 2;
    }

    reset_calls();
    (void)cnd_init(&cond);
    mutex.__state = 1;
    wait_results[0] = TEST_RAW_EINTR;
    wait_results[1] = 0L;
    wait_result_count = 2U;
    errno = EIO;
    if (cnd_wait(&cond, &mutex) != thrd_success || errno != EIO ||
        wait_calls != 2U || unlock_calls != 1U || lock_calls != 1U ||
        mutex.__state != 1) {
        return 3;
    }

    reset_calls();
    (void)cnd_init(&cond);
    mutex.__state = 1;
    wait_results[0] = TEST_RAW_EINVAL;
    wait_result_count = 1U;
    errno = EIO;
    if (cnd_wait(&cond, &mutex) != thrd_error || errno != EIO ||
        wait_calls != 1U || unlock_calls != 1U || lock_calls != 1U ||
        mutex.__state != 1) {
        return 4;
    }

    reset_calls();
    (void)cnd_init(&cond);
    mutex.__state = 1;
    fail_unlock = 1;
    errno = EIO;
    if (cnd_wait(&cond, &mutex) != thrd_error || errno != EIO ||
        unlock_calls != 1U || wait_calls != 0U || lock_calls != 0U ||
        mutex.__state != 1) {
        return 5;
    }

    reset_calls();
    (void)cnd_init(&cond);
    mutex.__state = 1;
    wait_results[0] = 0L;
    wait_result_count = 1U;
    fail_lock = 1;
    errno = EIO;
    if (cnd_wait(&cond, &mutex) != thrd_error || errno != EIO ||
        unlock_calls != 1U || wait_calls != 1U || lock_calls != 1U ||
        mutex.__state != 0) {
        return 6;
    }

    reset_calls();
    (void)cnd_init(&cond);
    wake_result = (int)TEST_RAW_EINVAL;
    errno = EIO;
    if (cnd_signal(&cond) != thrd_error || cond.__sequence != 1 ||
        errno != EIO || wake_calls != 1U || last_wake_count != 1) {
        return 7;
    }

    errno = EIO;
    if (cnd_init((cnd_t *)0) != thrd_error ||
        cnd_signal((cnd_t *)0) != thrd_error ||
        cnd_broadcast((cnd_t *)0) != thrd_error ||
        cnd_wait((cnd_t *)0, &mutex) != thrd_error ||
        cnd_wait(&cond, (mtx_t *)0) != thrd_error || errno != EIO) {
        return 8;
    }

    cond.__sequence = 19;
    cnd_destroy(&cond);
    if (cond.__sequence != 0) {
        return 9;
    }
    return 0;
}
