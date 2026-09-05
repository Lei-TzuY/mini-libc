#include <errno.h>
#include <mini/syscall.h>
#include <threads.h>

#define MINI_FUTEX_WAIT 0
#define MINI_FUTEX_WAKE 1
#define MINI_RAW_EINTR (-4L)
#define MINI_RAW_EAGAIN (-11L)
#define MINI_WAKE_ALL 0x7fffffff

extern int __mini_atomic_exchange_int(volatile int *value, int replacement);
extern int __mini_atomic_fetch_add_int(volatile int *value, int increment);

static int raw_failed(long value)
{
    return value < 0L && value >= -4095L;
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

int cnd_wait(cnd_t *cond, mtx_t *mtx)
{
    int saved_errno = errno;
    int expected;
    int wait_status = thrd_success;
    int lock_status;

    if (cond == (cnd_t *)0 || mtx == (mtx_t *)0) {
        errno = saved_errno;
        return thrd_error;
    }

    expected = __mini_atomic_fetch_add_int(
        (volatile int *)&cond->__sequence, 0);
    if (mtx_unlock(mtx) != thrd_success) {
        errno = saved_errno;
        return thrd_error;
    }

    for (;;) {
        long result = mini_sys_futex((volatile int *)&cond->__sequence,
                                     MINI_FUTEX_WAIT, expected,
                                     (const void *)0, (volatile int *)0, 0);

        if (!raw_failed(result) || result == MINI_RAW_EAGAIN) {
            break;
        }
        if (result == MINI_RAW_EINTR) {
            continue;
        }
        wait_status = thrd_error;
        break;
    }

    lock_status = mtx_lock(mtx);
    errno = saved_errno;
    if (wait_status != thrd_success || lock_status != thrd_success) {
        return thrd_error;
    }
    return thrd_success;
}

void cnd_destroy(cnd_t *cond)
{
    if (cond != (cnd_t *)0) {
        (void)__mini_atomic_exchange_int((volatile int *)&cond->__sequence, 0);
    }
}
