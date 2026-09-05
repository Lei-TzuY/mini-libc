#include <errno.h>
#include <mini/syscall.h>
#include <threads.h>

#define TEST_FUTEX_WAIT 0
#define TEST_FUTEX_WAKE 1
#define TEST_FUTEX_WAIT_BITSET 9
#define TEST_FUTEX_CLOCK_REALTIME 256
#define TEST_FUTEX_BITSET_MATCH_ANY (-1)
#define TEST_RAW_EINTR (-4L)
#define TEST_RAW_EAGAIN (-11L)
#define TEST_RAW_EINVAL (-22L)
#define TEST_RAW_ETIMEDOUT (-110L)
#define TEST_WAKE_ALL 0x7fffffff
#define TEST_THRD_SLEEP_ERROR (-2)

static unsigned int lock_calls;
static unsigned int unlock_calls;
static unsigned int wait_calls;
static unsigned int wake_calls;
static int last_wait_op;
static int last_wait_expected;
static int last_wait_value3;
static int last_wake_count;
static int last_wait_had_timeout;
static struct timespec last_wait_timeout;
static int fail_unlock;
static int fail_lock;
static int wake_result;
static long wait_results[4];
static unsigned int wait_result_count;
static unsigned int wait_result_index;
static cnd_t *signal_on_unlock_cond;

static unsigned int sleep_calls;
static struct timespec last_sleep_request;
static long sleep_result;
static struct timespec sleep_remaining_value;

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
    int timed_op = TEST_FUTEX_WAIT_BITSET | TEST_FUTEX_CLOCK_REALTIME;

    (void)uaddr2;

    if (op == TEST_FUTEX_WAIT || op == timed_op) {
        ++wait_calls;
        last_wait_op = op;
        last_wait_expected = value;
        last_wait_value3 = value3;
        last_wait_had_timeout = timeout != (const void *)0;
        if (timeout != (const void *)0) {
            last_wait_timeout = *(const struct timespec *)timeout;
        }
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

long mini_test_nanosleep(const void *request, void *remaining)
{
    ++sleep_calls;
    last_sleep_request = *(const struct timespec *)request;
    if (sleep_result == TEST_RAW_EINTR && remaining != (void *)0) {
        *(struct timespec *)remaining = sleep_remaining_value;
    }
    return sleep_result;
}

static void reset_calls(void)
{
    lock_calls = 0U;
    unlock_calls = 0U;
    wait_calls = 0U;
    wake_calls = 0U;
    last_wait_op = -1;
    last_wait_expected = -1;
    last_wait_value3 = 0;
    last_wake_count = -1;
    last_wait_had_timeout = 0;
    last_wait_timeout.tv_sec = 0;
    last_wait_timeout.tv_nsec = 0L;
    fail_unlock = 0;
    fail_lock = 0;
    wake_result = 0;
    wait_result_count = 0U;
    wait_result_index = 0U;
    signal_on_unlock_cond = (cnd_t *)0;
    sleep_calls = 0U;
    last_sleep_request.tv_sec = 0;
    last_sleep_request.tv_nsec = 0L;
    sleep_result = 0L;
    sleep_remaining_value.tv_sec = 0;
    sleep_remaining_value.tv_nsec = 0L;
}

int main(void)
{
    cnd_t cond;
    mtx_t mutex;
    struct timespec deadline;
    struct timespec duration;
    struct timespec remaining;

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
        wait_calls != 1U || last_wait_op != TEST_FUTEX_WAIT ||
        last_wait_expected != 0 || last_wait_had_timeout ||
        mutex.__state != 1) {
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

    deadline.tv_sec = 123;
    deadline.tv_nsec = 456789L;
    reset_calls();
    (void)cnd_init(&cond);
    mutex.__state = 1;
    signal_on_unlock_cond = &cond;
    errno = EIO;
    if (cnd_timedwait(&cond, &mutex, &deadline) != thrd_success ||
        errno != EIO || cond.__sequence != 1 || unlock_calls != 1U ||
        lock_calls != 1U || wait_calls != 1U ||
        last_wait_op != (TEST_FUTEX_WAIT_BITSET | TEST_FUTEX_CLOCK_REALTIME) ||
        last_wait_expected != 0 || !last_wait_had_timeout ||
        last_wait_timeout.tv_sec != deadline.tv_sec ||
        last_wait_timeout.tv_nsec != deadline.tv_nsec ||
        last_wait_value3 != TEST_FUTEX_BITSET_MATCH_ANY ||
        mutex.__state != 1) {
        return 10;
    }

    reset_calls();
    (void)cnd_init(&cond);
    mutex.__state = 1;
    wait_results[0] = TEST_RAW_ETIMEDOUT;
    wait_result_count = 1U;
    errno = EIO;
    if (cnd_timedwait(&cond, &mutex, &deadline) != thrd_timedout ||
        errno != EIO || wait_calls != 1U || unlock_calls != 1U ||
        lock_calls != 1U || mutex.__state != 1) {
        return 11;
    }

    reset_calls();
    (void)cnd_init(&cond);
    mutex.__state = 1;
    wait_results[0] = TEST_RAW_EINTR;
    wait_results[1] = TEST_RAW_ETIMEDOUT;
    wait_result_count = 2U;
    errno = EIO;
    if (cnd_timedwait(&cond, &mutex, &deadline) != thrd_timedout ||
        errno != EIO || wait_calls != 2U || unlock_calls != 1U ||
        lock_calls != 1U || mutex.__state != 1) {
        return 12;
    }

    reset_calls();
    (void)cnd_init(&cond);
    mutex.__state = 1;
    wait_results[0] = TEST_RAW_EINVAL;
    wait_result_count = 1U;
    errno = EIO;
    if (cnd_timedwait(&cond, &mutex, &deadline) != thrd_error ||
        errno != EIO || wait_calls != 1U || unlock_calls != 1U ||
        lock_calls != 1U || mutex.__state != 1) {
        return 13;
    }

    reset_calls();
    (void)cnd_init(&cond);
    mutex.__state = 1;
    wait_results[0] = TEST_RAW_ETIMEDOUT;
    wait_result_count = 1U;
    fail_lock = 1;
    errno = EIO;
    if (cnd_timedwait(&cond, &mutex, &deadline) != thrd_error ||
        errno != EIO || wait_calls != 1U || unlock_calls != 1U ||
        lock_calls != 1U || mutex.__state != 0) {
        return 14;
    }

    reset_calls();
    (void)cnd_init(&cond);
    mutex.__state = 1;
    deadline.tv_sec = -1;
    deadline.tv_nsec = 0L;
    errno = EIO;
    if (cnd_timedwait(&cond, &mutex, &deadline) != thrd_timedout ||
        errno != EIO || wait_calls != 0U || unlock_calls != 1U ||
        lock_calls != 1U || mutex.__state != 1) {
        return 15;
    }

    reset_calls();
    (void)cnd_init(&cond);
    mutex.__state = 1;
    deadline.tv_sec = 1;
    deadline.tv_nsec = 1000000000L;
    errno = EIO;
    if (cnd_timedwait(&cond, &mutex, &deadline) != thrd_error ||
        cnd_timedwait(&cond, &mutex, (const struct timespec *)0) != thrd_error ||
        cnd_timedwait((cnd_t *)0, &mutex, &deadline) != thrd_error ||
        cnd_timedwait(&cond, (mtx_t *)0, &deadline) != thrd_error ||
        errno != EIO || wait_calls != 0U || unlock_calls != 0U ||
        lock_calls != 0U || mutex.__state != 1) {
        return 16;
    }

    reset_calls();
    duration.tv_sec = 0;
    duration.tv_nsec = 1000000L;
    remaining.tv_sec = 7;
    remaining.tv_nsec = 8L;
    errno = EIO;
    if (thrd_sleep(&duration, &remaining) != 0 || errno != EIO ||
        sleep_calls != 1U || last_sleep_request.tv_sec != 0 ||
        last_sleep_request.tv_nsec != 1000000L || remaining.tv_sec != 7 ||
        remaining.tv_nsec != 8L) {
        return 17;
    }

    reset_calls();
    duration.tv_sec = 0;
    duration.tv_nsec = 900000L;
    remaining.tv_sec = 0;
    remaining.tv_nsec = 0L;
    sleep_result = TEST_RAW_EINTR;
    sleep_remaining_value.tv_sec = 0;
    sleep_remaining_value.tv_nsec = 300000L;
    errno = EIO;
    if (thrd_sleep(&duration, &remaining) != -1 || errno != EIO ||
        sleep_calls != 1U || remaining.tv_sec != 0 ||
        remaining.tv_nsec != 300000L) {
        return 18;
    }

    reset_calls();
    duration.tv_sec = 0;
    duration.tv_nsec = 900000L;
    sleep_result = TEST_RAW_EINTR;
    sleep_remaining_value.tv_sec = 0;
    sleep_remaining_value.tv_nsec = 250000L;
    errno = EIO;
    if (thrd_sleep(&duration, &duration) != -1 || errno != EIO ||
        sleep_calls != 1U || last_sleep_request.tv_nsec != 900000L ||
        duration.tv_sec != 0 || duration.tv_nsec != 250000L) {
        return 19;
    }

    reset_calls();
    duration.tv_sec = -1;
    duration.tv_nsec = 0L;
    errno = EIO;
    if (thrd_sleep(&duration, (struct timespec *)0) != TEST_THRD_SLEEP_ERROR ||
        thrd_sleep((const struct timespec *)0, (struct timespec *)0) !=
            TEST_THRD_SLEEP_ERROR ||
        sleep_calls != 0U || errno != EIO) {
        return 20;
    }

    reset_calls();
    duration.tv_sec = 0;
    duration.tv_nsec = 1000000000L;
    errno = EIO;
    if (thrd_sleep(&duration, (struct timespec *)0) != TEST_THRD_SLEEP_ERROR ||
        sleep_calls != 0U || errno != EIO) {
        return 21;
    }

    reset_calls();
    duration.tv_sec = 0;
    duration.tv_nsec = 1L;
    sleep_result = TEST_RAW_EINVAL;
    remaining.tv_sec = 9;
    remaining.tv_nsec = 10L;
    errno = EIO;
    if (thrd_sleep(&duration, &remaining) != TEST_THRD_SLEEP_ERROR ||
        sleep_calls != 1U || remaining.tv_sec != 9 || remaining.tv_nsec != 10L ||
        errno != EIO) {
        return 22;
    }

    return 0;
}
