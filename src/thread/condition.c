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
#define MINI_WAKE_ALL 0x7fffffff
#define MINI_NSEC_PER_SEC 1000000000L

extern int __mini_atomic_exchange_int(volatile int *value, int replacement);
extern int __mini_atomic_fetch_add_int(volatile int *value, int increment);

static int raw_failed(long value)
{
    return value < 0L && value >= -4095L;
}

static int valid_time_point(const struct timespec *time_point)
{
    return time_point != (const struct timespec *)0 &&
           time_point->tv_nsec >= 0L &&
           time_point->tv_nsec < MINI_NSEC_PER_SEC;
}

int cnd_init(cnd_t *cond)
{
    int saved_errno = errno;

    if (cond == (cnd_t *)0) {
        errno = saved_errno;
        return thrd_error;
    }
    (void)__mini_atomic_exchange_int((volatile int *)&cond->__sequence, 0);
    errno = saved_errno;
    return thrd_success;
}

static int wake_waiters(cnd_t *cond, int count)
{
    int saved_errno = errno;
    long result;

    if (cond == (cnd_t *)0) {
        errno = saved_errno;
        return thrd_error;
    }

    (void)__mini_atomic_fetch_add_int((volatile int *)&cond->__sequence, 1);
    result = mini_sys_futex((volatile int *)&cond->__sequence, MINI_FUTEX_WAKE,
                            count, (const void *)0, (volatile int *)0, 0);
    errno = saved_errno;
    return raw_failed(result) ? thrd_error : thrd_success;
}

int cnd_signal(cnd_t *cond)
{
    return wake_waiters(cond, 1);
}

int cnd_broadcast(cnd_t *cond)
{
    return wake_waiters(cond, MINI_WAKE_ALL);
}

static int wait_common(cnd_t *cond, mtx_t *mtx,
                       const struct timespec *time_point, int timed)
{
    int saved_errno = errno;
    int expected;
    int wait_status = thrd_success;
    int lock_status;

    if (cond == (cnd_t *)0 || mtx == (mtx_t *)0 ||
        (timed && !valid_time_point(time_point))) {
        errno = saved_errno;
        return thrd_error;
    }
    if ((mtx->__type & mtx_recursive) != 0 && mtx->__depth != 1) {
        errno = saved_errno;
        return thrd_error;
    }

    expected = __mini_atomic_fetch_add_int(
        (volatile int *)&cond->__sequence, 0);
    if (mtx_unlock(mtx) != thrd_success) {
        errno = saved_errno;
        return thrd_error;
    }

    if (timed && time_point->tv_sec < 0) {
        wait_status = thrd_timedout;
    } else {
        for (;;) {
            int op = timed ? MINI_FUTEX_WAIT_BITSET |
                                 MINI_FUTEX_CLOCK_REALTIME
                           : MINI_FUTEX_WAIT;
            const void *timeout = timed ? (const void *)time_point
                                        : (const void *)0;
            int bitset = timed ? MINI_FUTEX_BITSET_MATCH_ANY : 0;
            long result = mini_sys_futex(
                (volatile int *)&cond->__sequence, op, expected, timeout,
                (volatile int *)0, bitset);

            if (!raw_failed(result) || result == MINI_RAW_EAGAIN) {
                break;
            }
            if (result == MINI_RAW_EINTR) {
                continue;
            }
            if (timed && result == MINI_RAW_ETIMEDOUT) {
                wait_status = thrd_timedout;
            } else {
                wait_status = thrd_error;
            }
            break;
        }
    }

    lock_status = mtx_lock(mtx);
    errno = saved_errno;
    if (lock_status != thrd_success) {
        return thrd_error;
    }
    return wait_status;
}

int cnd_wait(cnd_t *cond, mtx_t *mtx)
{
    return wait_common(cond, mtx, (const struct timespec *)0, 0);
}

int cnd_timedwait(cnd_t *restrict cond, mtx_t *restrict mtx,
                  const struct timespec *restrict time_point)
{
    return wait_common(cond, mtx, time_point, 1);
}

void cnd_destroy(cnd_t *cond)
{
    if (cond != (cnd_t *)0) {
        (void)__mini_atomic_exchange_int((volatile int *)&cond->__sequence, 0);
    }
}
