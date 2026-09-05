#include <errno.h>
#include <mini/syscall.h>
#include <threads.h>

#define WORKER_COUNT 8

struct worker_arg {
    int id;
    int destructor_calls;
};

static once_flag once_control = ONCE_FLAG_INIT;
static tss_t worker_key;
static mtx_t barrier_mutex;
static cnd_t barrier_cond;
static int barrier_ready;
static int barrier_open;
static int once_calls;
static int once_value;

static void once_initializer(void)
{
    static const struct timespec delay = {0, 20000000L};

    ++once_calls;
    (void)thrd_sleep(&delay, (struct timespec *)0);
    once_value = 0x1234;
}

static void worker_destructor(void *opaque)
{
    struct worker_arg *arg = (struct worker_arg *)opaque;

    ++arg->destructor_calls;
    if (arg->destructor_calls < 3) {
        (void)tss_set(worker_key, arg);
    }
}

static int wait_at_barrier(void)
{
    if (mtx_lock(&barrier_mutex) != thrd_success) {
        return 0;
    }
    ++barrier_ready;
    if (barrier_ready == WORKER_COUNT) {
        barrier_open = 1;
        if (cnd_broadcast(&barrier_cond) != thrd_success) {
            (void)mtx_unlock(&barrier_mutex);
            return 0;
        }
    } else {
        while (!barrier_open) {
            if (cnd_wait(&barrier_cond, &barrier_mutex) != thrd_success) {
                (void)mtx_unlock(&barrier_mutex);
                return 0;
            }
        }
    }
    return mtx_unlock(&barrier_mutex) == thrd_success;
}

static int worker(void *opaque)
{
    struct worker_arg *arg = (struct worker_arg *)opaque;

    if (!wait_at_barrier()) {
        return -10;
    }
    call_once(&once_control, once_initializer);
    if (once_value != 0x1234 || tss_get(worker_key) != (void *)0) {
        return -11;
    }
    errno = 50 + arg->id;
    if (tss_set(worker_key, arg) != thrd_success ||
        tss_get(worker_key) != arg || errno != 50 + arg->id) {
        return -12;
    }
    return 0;
}

static int explicit_exit_worker(void *opaque)
{
    struct worker_arg *arg = (struct worker_arg *)opaque;

    call_once(&once_control, once_initializer);
    if (tss_set(worker_key, arg) != thrd_success) {
        return -20;
    }
    thrd_exit(77);
    return 0;
}

int main(void)
{
    static const char marker[] = "once-tss-ok";
    struct worker_arg args[WORKER_COUNT];
    struct worker_arg explicit_arg = {99, 0};
    thrd_t threads[WORKER_COUNT];
    thrd_t explicit_thread;
    int i;

    errno = EIO;
    if (mtx_init(&barrier_mutex, mtx_plain) != thrd_success ||
        cnd_init(&barrier_cond) != thrd_success ||
        tss_create(&worker_key, worker_destructor) != thrd_success ||
        errno != EIO) {
        return 1;
    }

    for (i = 0; i < WORKER_COUNT; ++i) {
        args[i].id = i;
        args[i].destructor_calls = 0;
        if (thrd_create(&threads[i], worker, &args[i]) != thrd_success) {
            return 2;
        }
    }
    for (i = 0; i < WORKER_COUNT; ++i) {
        int result;

        if (thrd_join(threads[i], &result) != thrd_success || result != 0 ||
            args[i].destructor_calls != 3) {
            return 3;
        }
    }
    if (once_calls != 1 || once_value != 0x1234 ||
        tss_get(worker_key) != (void *)0) {
        return 4;
    }

    if (thrd_create(&explicit_thread, explicit_exit_worker, &explicit_arg) !=
        thrd_success) {
        return 5;
    }
    {
        int result;

        if (thrd_join(explicit_thread, &result) != thrd_success || result != 77 ||
            explicit_arg.destructor_calls != 3) {
            return 6;
        }
    }

    call_once(&once_control, once_initializer);
    if (once_calls != 1 || errno != EIO) {
        return 7;
    }

    tss_delete(worker_key);
    if (errno != EIO) {
        return 8;
    }
    cnd_destroy(&barrier_cond);
    mtx_destroy(&barrier_mutex);

    if (mini_sys_write(1, marker, sizeof(marker) - 1U) !=
        (long)(sizeof(marker) - 1U)) {
        return 9;
    }
    return 0;
}
