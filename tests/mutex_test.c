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
#define TEST_INT_MAX ((int)(~0U >> 1))

struct mini_thread_tcb {
    struct mini_thread_tcb *self;
    void *control;
    int errno_value;
    unsigned int reserved;
};

static struct mini_thread_tcb fake_tcbs[3];
static unsigned int current_tcb;
static unsigned int wait_calls;
static unsigned int wake_calls;
static int last_wait_op;
static int last_wait_expected;
static int last_wait_value3;
static int last_wait_had_timeout;
static struct timespec last_wait_timeout;
static long wait_results[4];
static int release_on_wait[4];
static unsigned int wait_result_count;
static unsigned int wait_result_index;

struct mini_thread_tcb *__mini_thread_current_tcb(void)
{
    return &fake_tcbs[current_tcb];
}

long mini_sys_futex(volatile int *uaddr, int op, int value,
                    const void *timeout, volatile int *uaddr2, int value3)
{
    int timed_op = TEST_FUTEX_WAIT_BITSET | TEST_FUTEX_CLOCK_REALTIME;

    (void)uaddr2;
    if (op == TEST_FUTEX_WAIT || op == timed_op) {
        unsigned int index = wait_result_index;

        ++wait_calls;
        last_wait_op = op;
        last_wait_expected = value;
        last_wait_value3 = value3;
        last_wait_had_timeout = timeout != (const void *)0;
        if (timeout != (const void *)0) {
            last_wait_timeout = *(const struct timespec *)timeout;
        }
        if (index < wait_result_count) {
            ++wait_result_index;
            if (release_on_wait[index]) {
                *uaddr = 0;
            }
            return wait_results[index];
        }
        return *uaddr == value ? 0L : TEST_RAW_EAGAIN;
    }
    if (op == TEST_FUTEX_WAKE) {
        ++wake_calls;
        return 1L;
    }
    return TEST_RAW_EINVAL;
}

static void reset_calls(void)
{
    unsigned int i;

    wait_calls = 0U;
    wake_calls = 0U;
    last_wait_op = -1;
    last_wait_expected = -1;
    last_wait_value3 = 0;
    last_wait_had_timeout = 0;
    last_wait_timeout.tv_sec = 0;
    last_wait_timeout.tv_nsec = 0L;
    wait_result_count = 0U;
    wait_result_index = 0U;
    for (i = 0U; i < 4U; ++i) {
        wait_results[i] = 0L;
        release_on_wait[i] = 0;
    }
}

static void make_busy(mtx_t *mtx, int type, unsigned int owner_index)
{
    mtx->__state = 1;
    mtx->__type = type;
    mtx->__owner = (unsigned long)&fake_tcbs[owner_index];
    mtx->__depth = 1;
}

int main(void)
{
    mtx_t mutex;
    struct timespec deadline;

    fake_tcbs[0].self = &fake_tcbs[0];
    fake_tcbs[1].self = &fake_tcbs[1];
    fake_tcbs[2].self = &fake_tcbs[2];
    current_tcb = 0U;
    reset_calls();

    errno = EIO;
    if (mtx_init(&mutex, mtx_plain) != thrd_success || errno != EIO ||
        mutex.__state != 0 || mutex.__type != mtx_plain ||
        mutex.__owner != 0UL || mutex.__depth != 0 ||
        mtx_init((mtx_t *)0, mtx_plain) != thrd_error ||
        mtx_init(&mutex, 4) != thrd_error || errno != EIO) {
        return 1;
    }

    if (mtx_init(&mutex, mtx_recursive | mtx_timed) != thrd_success ||
        mutex.__type != (mtx_recursive | mtx_timed) || errno != EIO) {
        return 2;
    }

    reset_calls();
    if (mtx_init(&mutex, mtx_plain) != thrd_success ||
        mtx_trylock(&mutex) != thrd_success || mutex.__state != 1 ||
        mutex.__owner != (unsigned long)&fake_tcbs[0] || mutex.__depth != 1 ||
        mtx_trylock(&mutex) != thrd_busy ||
        mtx_lock(&mutex) != thrd_error || errno != EIO) {
        return 3;
    }
    current_tcb = 1U;
    if (mtx_unlock(&mutex) != thrd_error || mutex.__state != 1 ||
        mutex.__owner != (unsigned long)&fake_tcbs[0] || wake_calls != 0U ||
        errno != EIO) {
        return 4;
    }
    current_tcb = 0U;
    if (mtx_unlock(&mutex) != thrd_success || mutex.__state != 0 ||
        mutex.__owner != 0UL || mutex.__depth != 0 || wake_calls != 1U ||
        errno != EIO) {
        return 5;
    }

    reset_calls();
    if (mtx_init(&mutex, mtx_recursive) != thrd_success ||
        mtx_lock(&mutex) != thrd_success ||
        mtx_lock(&mutex) != thrd_success ||
        mtx_trylock(&mutex) != thrd_success || mutex.__depth != 3 ||
        wait_calls != 0U || wake_calls != 0U || errno != EIO) {
        return 6;
    }
    if (mtx_unlock(&mutex) != thrd_success || mutex.__depth != 2 ||
        mutex.__state != 1 || wake_calls != 0U ||
        mtx_unlock(&mutex) != thrd_success || mutex.__depth != 1 ||
        mutex.__state != 1 || wake_calls != 0U ||
        mtx_unlock(&mutex) != thrd_success || mutex.__state != 0 ||
        mutex.__depth != 0 || wake_calls != 1U || errno != EIO) {
        return 7;
    }

    reset_calls();
    make_busy(&mutex, mtx_recursive, 0U);
    mutex.__depth = TEST_INT_MAX;
    if (mtx_lock(&mutex) != thrd_error || mutex.__depth != TEST_INT_MAX ||
        wait_calls != 0U || errno != EIO) {
        return 8;
    }

    deadline.tv_sec = 10;
    deadline.tv_nsec = 20L;
    reset_calls();
    if (mtx_init(&mutex, mtx_plain) != thrd_success ||
        mtx_timedlock(&mutex, &deadline) != thrd_error ||
        mtx_timedlock((mtx_t *)0, &deadline) != thrd_error ||
        wait_calls != 0U || errno != EIO) {
        return 9;
    }

    reset_calls();
    if (mtx_init(&mutex, mtx_timed) != thrd_success) {
        return 10;
    }
    deadline.tv_sec = 1;
    deadline.tv_nsec = 1000000000L;
    if (mtx_timedlock(&mutex, &deadline) != thrd_error ||
        mtx_timedlock(&mutex, (const struct timespec *)0) != thrd_error ||
        wait_calls != 0U || errno != EIO) {
        return 11;
    }

    reset_calls();
    deadline.tv_sec = -1;
    deadline.tv_nsec = 0L;
    if (mtx_init(&mutex, mtx_timed) != thrd_success ||
        mtx_timedlock(&mutex, &deadline) != thrd_success ||
        wait_calls != 0U || mutex.__state != 1 ||
        mtx_unlock(&mutex) != thrd_success || errno != EIO) {
        return 12;
    }

    reset_calls();
    make_busy(&mutex, mtx_timed, 1U);
    if (mtx_timedlock(&mutex, &deadline) != thrd_timedout ||
        wait_calls != 0U || mutex.__owner != (unsigned long)&fake_tcbs[1] ||
        errno != EIO) {
        return 13;
    }

    reset_calls();
    make_busy(&mutex, mtx_timed, 1U);
    deadline.tv_sec = 123;
    deadline.tv_nsec = 456789L;
    wait_results[0] = TEST_RAW_ETIMEDOUT;
    wait_result_count = 1U;
    if (mtx_timedlock(&mutex, &deadline) != thrd_timedout ||
        wait_calls != 1U ||
        last_wait_op != (TEST_FUTEX_WAIT_BITSET | TEST_FUTEX_CLOCK_REALTIME) ||
        last_wait_expected != 1 || !last_wait_had_timeout ||
        last_wait_timeout.tv_sec != deadline.tv_sec ||
        last_wait_timeout.tv_nsec != deadline.tv_nsec ||
        last_wait_value3 != TEST_FUTEX_BITSET_MATCH_ANY || errno != EIO) {
        return 14;
    }

    reset_calls();
    make_busy(&mutex, mtx_timed, 1U);
    wait_results[0] = TEST_RAW_EINTR;
    wait_results[1] = TEST_RAW_EAGAIN;
    release_on_wait[1] = 1;
    wait_result_count = 2U;
    if (mtx_timedlock(&mutex, &deadline) != thrd_success ||
        wait_calls != 2U || mutex.__state != 1 ||
        mutex.__owner != (unsigned long)&fake_tcbs[0] || mutex.__depth != 1 ||
        mtx_unlock(&mutex) != thrd_success || errno != EIO) {
        return 15;
    }

    reset_calls();
    make_busy(&mutex, mtx_timed, 1U);
    wait_results[0] = TEST_RAW_EINVAL;
    wait_result_count = 1U;
    if (mtx_timedlock(&mutex, &deadline) != thrd_error ||
        wait_calls != 1U || mutex.__state != 1 || errno != EIO) {
        return 16;
    }

    reset_calls();
    make_busy(&mutex, mtx_plain, 1U);
    wait_results[0] = TEST_RAW_EAGAIN;
    release_on_wait[0] = 1;
    wait_result_count = 1U;
    if (mtx_lock(&mutex) != thrd_success || wait_calls != 1U ||
        last_wait_op != TEST_FUTEX_WAIT || last_wait_had_timeout ||
        mutex.__owner != (unsigned long)&fake_tcbs[0] ||
        mtx_unlock(&mutex) != thrd_success || errno != EIO) {
        return 17;
    }

    reset_calls();
    deadline.tv_sec = -1;
    deadline.tv_nsec = 0L;
    if (mtx_init(&mutex, mtx_recursive | mtx_timed) != thrd_success ||
        mtx_lock(&mutex) != thrd_success ||
        mtx_timedlock(&mutex, &deadline) != thrd_success ||
        mutex.__depth != 2 || wait_calls != 0U ||
        mtx_unlock(&mutex) != thrd_success || mutex.__depth != 1 ||
        mtx_unlock(&mutex) != thrd_success || errno != EIO) {
        return 18;
    }

    mutex.__state = 1;
    mutex.__type = mtx_timed;
    mutex.__owner = 99UL;
    mutex.__depth = 7;
    mtx_destroy(&mutex);
    if (mutex.__state != 0 || mutex.__type != mtx_plain ||
        mutex.__owner != 0UL || mutex.__depth != 0 || errno != EIO) {
        return 19;
    }

    return 0;
}
