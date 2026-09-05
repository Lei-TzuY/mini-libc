#include <errno.h>
#include <mini/syscall.h>
#include <threads.h>

#define CONDITION_WORKERS 6

struct condition_worker_arg {
    int id;
    int errno_value;
};

static mtx_t condition_mutex;
static cnd_t gate_condition;
static cnd_t progress_condition;
static int ready_workers;
static int available_tokens;
static int completed_workers;

static int condition_worker(void *opaque)
{
    struct condition_worker_arg *arg = (struct condition_worker_arg *)opaque;
    int result;

    errno = arg->errno_value;
    if (mtx_lock(&condition_mutex) != thrd_success ||
        errno != arg->errno_value) {
        return -1;
    }

    ++ready_workers;
    if (cnd_signal(&progress_condition) != thrd_success ||
        errno != arg->errno_value) {
        (void)mtx_unlock(&condition_mutex);
        return -2;
    }

    while (available_tokens == 0) {
        if (cnd_wait(&gate_condition, &condition_mutex) != thrd_success ||
            errno != arg->errno_value) {
            (void)mtx_unlock(&condition_mutex);
            return -3;
        }
    }

    --available_tokens;
    ++completed_workers;
    if (cnd_signal(&progress_condition) != thrd_success ||
        errno != arg->errno_value ||
        mtx_unlock(&condition_mutex) != thrd_success ||
        errno != arg->errno_value) {
        return -4;
    }

    result = 100 + arg->id;
    return result;
}

int main(void)
{
    static const char marker[] = "conditions-ok";
    struct condition_worker_arg args[CONDITION_WORKERS];
    thrd_t workers[CONDITION_WORKERS];
    int i;

    errno = EIO;
    if (mtx_init(&condition_mutex, mtx_plain) != thrd_success ||
        cnd_init(&gate_condition) != thrd_success ||
        cnd_init(&progress_condition) != thrd_success || errno != EIO ||
        mtx_lock(&condition_mutex) != thrd_success || errno != EIO) {
        return 1;
    }

    for (i = 0; i < CONDITION_WORKERS; ++i) {
        args[i].id = i;
        args[i].errno_value = 71 + i;
        if (thrd_create(&workers[i], condition_worker, &args[i]) !=
                thrd_success ||
            errno != EIO) {
            return 2;
        }
    }

    while (ready_workers != CONDITION_WORKERS) {
        if (cnd_wait(&progress_condition, &condition_mutex) != thrd_success ||
            errno != EIO) {
            return 3;
        }
    }

    available_tokens = 1;
    if (cnd_signal(&gate_condition) != thrd_success || errno != EIO) {
        return 4;
    }
    while (completed_workers != 1) {
        if (cnd_wait(&progress_condition, &condition_mutex) != thrd_success ||
            errno != EIO) {
            return 5;
        }
    }

    available_tokens = CONDITION_WORKERS - 1;
    if (cnd_broadcast(&gate_condition) != thrd_success || errno != EIO) {
        return 6;
    }
    while (completed_workers != CONDITION_WORKERS) {
        if (cnd_wait(&progress_condition, &condition_mutex) != thrd_success ||
            errno != EIO) {
            return 7;
        }
    }

    if (available_tokens != 0 ||
        mtx_unlock(&condition_mutex) != thrd_success || errno != EIO) {
        return 8;
    }

    for (i = 0; i < CONDITION_WORKERS; ++i) {
        int result = -1;

        if (thrd_join(workers[i], &result) != thrd_success ||
            result != 100 + i || errno != EIO) {
            return 9;
        }
    }

    cnd_destroy(&progress_condition);
    cnd_destroy(&gate_condition);
    mtx_destroy(&condition_mutex);
    if (mini_sys_write(1, marker, sizeof(marker) - 1U) !=
        (long)(sizeof(marker) - 1U)) {
        return 10;
    }
    return 0;
}
