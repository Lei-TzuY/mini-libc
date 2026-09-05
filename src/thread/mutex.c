#include <errno.h>
#include <mini/syscall.h>
#include <threads.h>

#define MINI_FUTEX_WAIT 0
#define MINI_FUTEX_WAKE 1
#define MINI_FUTEX_WAIT_BITSET 9
#define MINI_FUTEX_CLOCK_REALTIME 256
#define MINI_FUTEX_BITSET_MATCH_ANY (-1)
#define MINI_RAW_EINTR (-4L)
#define MINI_RAW_EAGAIN (-11L)
#define MINI_RAW_ETIMEDOUT (-110L)
#define MINI_NSEC_PER_SEC 1000000000L
#define MINI_INT_MAX ((int)(~0U >> 1))
#define MINI_MTX_TYPE_MASK (mtx_recursive | mtx_timed)

extern int __mini_atomic_exchange_int(volatile int *value, int replacement);
extern int __mini_atomic_fetch_add_int(volatile int *value, int increment);

static int atomic_load_int(volatile int *value)
{
    return __mini_atomic_fetch_add_int(value, 0);
}

static int valid_type(int type)
{
    return type >= 0 && (type & ~MINI_MTX_TYPE_MASK) == 0;
}

static int valid_time_point(const struct timespec *time_point)
{
    return time_point != (const struct timespec *)0 &&
           time_point->tv_nsec >= 0L &&
           time_point->tv_nsec < MINI_NSEC_PER_SEC;
}

static int current_tid(void)
{
    thrd_t current = thrd_current();

    if (current == (thrd_t)0 || current > (thrd_t)(unsigned int)MINI_INT_MAX) {
        return 0;
    }
    return (int)(unsigned int)current;
}

static int owned_by(mtx_t *mtx, int tid)
{
    return atomic_load_int((volatile int *)&mtx->__state) != 0 &&
           atomic_load_int((volatile int *)&mtx->__owner) == tid;
}

static int acquire_unlocked(mtx_t *mtx, int tid)
{
    if (__mini_atomic_exchange_int((volatile int *)&mtx->__state, 1) != 0) {
        return 0;
    }
    (void)__mini_atomic_exchange_int((volatile int *)&mtx->__owner, tid);
    (void)__mini_atomic_exchange_int((volatile int *)&mtx->__depth, 1);
    return 1;
}

static int recurse_owned(mtx_t *mtx)
{
    int depth = atomic_load_int((volatile int *)&mtx->__depth);

    if (depth <= 0 || depth == MINI_INT_MAX) {
        return thrd_error;
    }
    (void)__mini_atomic_fetch_add_int((volatile int *)&mtx->__depth, 1);
    return thrd_success;
}

static int lock_common(mtx_t *mtx, const struct timespec *time_point, int timed)
{
    int saved_errno = errno;
    int tid;

    if (mtx == (mtx_t *)0 || !valid_type(mtx->__type) ||
        (timed && (((mtx->__type & mtx_timed) == 0) ||
                   !valid_time_point(time_point)))) {
        errno = saved_errno;
        return thrd_error;
    }

    tid = current_tid();
    if (tid == 0) {
        errno = saved_errno;
        return thrd_error;
    }

    if ((mtx->__type & mtx_recursive) != 0 && owned_by(mtx, tid)) {
        int result = recurse_owned(mtx);

        errno = saved_errno;
        return result;
    }
    if ((mtx->__type & mtx_recursive) == 0 && owned_by(mtx, tid)) {
        errno = saved_errno;
        return thrd_error;
    }

    for (;;) {
        long wait_result;

        if (acquire_unlocked(mtx, tid)) {
            errno = saved_errno;
            return thrd_success;
        }
        if (timed && time_point->tv_sec < 0L) {
            errno = saved_errno;
            return thrd_timedout;
        }

        if (timed) {
            wait_result = mini_sys_futex(
                (volatile int *)&mtx->__state,
                MINI_FUTEX_WAIT_BITSET | MINI_FUTEX_CLOCK_REALTIME, 1,
                (const void *)time_point, (volatile int *)0,
                MINI_FUTEX_BITSET_MATCH_ANY);
        } else {
            wait_result = mini_sys_futex((volatile int *)&mtx->__state,
                                         MINI_FUTEX_WAIT, 1,
                                         (const void *)0,
                                         (volatile int *)0, 0);
        }

        if (wait_result >= 0L || wait_result == MINI_RAW_EAGAIN) {
            continue;
        }
        if (wait_result == MINI_RAW_EINTR) {
            continue;
        }
        if (timed && wait_result == MINI_RAW_ETIMEDOUT) {
            errno = saved_errno;
            return thrd_timedout;
        }
        errno = saved_errno;
        return thrd_error;
    }
}

int mtx_init(mtx_t *mtx, int type)
{
    int saved_errno = errno;

    if (mtx == (mtx_t *)0 || !valid_type(type)) {
        errno = saved_errno;
        return thrd_error;
    }

    (void)__mini_atomic_exchange_int((volatile int *)&mtx->__state, 0);
    mtx->__type = type;
    (void)__mini_atomic_exchange_int((volatile int *)&mtx->__owner, 0);
    (void)__mini_atomic_exchange_int((volatile int *)&mtx->__depth, 0);
    errno = saved_errno;
    return thrd_success;
}

int mtx_trylock(mtx_t *mtx)
{
    int saved_errno = errno;
    int tid;

    if (mtx == (mtx_t *)0 || !valid_type(mtx->__type)) {
        errno = saved_errno;
        return thrd_error;
    }
    tid = current_tid();
    if (tid == 0) {
        errno = saved_errno;
        return thrd_error;
    }

    if ((mtx->__type & mtx_recursive) != 0 && owned_by(mtx, tid)) {
        int result = recurse_owned(mtx);

        errno = saved_errno;
        return result;
    }
    if (owned_by(mtx, tid)) {
        errno = saved_errno;
        return thrd_busy;
    }
    if (acquire_unlocked(mtx, tid)) {
        errno = saved_errno;
        return thrd_success;
    }

    errno = saved_errno;
    return thrd_busy;
}

int mtx_lock(mtx_t *mtx)
{
    return lock_common(mtx, (const struct timespec *)0, 0);
}

int mtx_timedlock(mtx_t *restrict mtx,
                  const struct timespec *restrict time_point)
{
    return lock_common(mtx, time_point, 1);
}

int mtx_unlock(mtx_t *mtx)
{
    int saved_errno = errno;
    int tid;
    int depth;

    if (mtx == (mtx_t *)0 || !valid_type(mtx->__type)) {
        errno = saved_errno;
        return thrd_error;
    }
    tid = current_tid();
    if (tid == 0 || !owned_by(mtx, tid)) {
        errno = saved_errno;
        return thrd_error;
    }

    depth = atomic_load_int((volatile int *)&mtx->__depth);
    if (depth <= 0) {
        errno = saved_errno;
        return thrd_error;
    }
    if ((mtx->__type & mtx_recursive) != 0 && depth > 1) {
        (void)__mini_atomic_fetch_add_int((volatile int *)&mtx->__depth, -1);
        errno = saved_errno;
        return thrd_success;
    }
    if (depth != 1) {
        errno = saved_errno;
        return thrd_error;
    }

    (void)__mini_atomic_exchange_int((volatile int *)&mtx->__depth, 0);
    (void)__mini_atomic_exchange_int((volatile int *)&mtx->__owner, 0);
    if (__mini_atomic_exchange_int((volatile int *)&mtx->__state, 0) == 0) {
        errno = saved_errno;
        return thrd_error;
    }
    (void)mini_sys_futex((volatile int *)&mtx->__state, MINI_FUTEX_WAKE, 1,
                         (const void *)0, (volatile int *)0, 0);
    errno = saved_errno;
    return thrd_success;
}

void mtx_destroy(mtx_t *mtx)
{
    int saved_errno = errno;

    if (mtx != (mtx_t *)0) {
        (void)__mini_atomic_exchange_int((volatile int *)&mtx->__depth, 0);
        (void)__mini_atomic_exchange_int((volatile int *)&mtx->__owner, 0);
        (void)__mini_atomic_exchange_int((volatile int *)&mtx->__state, 0);
        mtx->__type = mtx_plain;
    }
    errno = saved_errno;
}
