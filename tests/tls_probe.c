#include <errno.h>
#include <mini/syscall.h>
#include <threads.h>

#define TLS_WORKERS 4

_Alignas(16) thread_local int tls_initialized = 41;
thread_local int tls_zero;
thread_local unsigned long tls_cookie = 0x1122334455667788UL;

static mtx_t tls_gate;

struct tls_worker_arg {
    int id;
};

static int tls_worker(void *opaque)
{
    struct tls_worker_arg *arg = (struct tls_worker_arg *)opaque;
    int expected_initialized = 100 + arg->id;
    int expected_zero = 200 + arg->id;
    unsigned long expected_cookie = 0xabc00000UL + (unsigned long)arg->id;

    if (tls_initialized != 41 || tls_zero != 0 ||
        tls_cookie != 0x1122334455667788UL ||
        ((unsigned long)&tls_initialized & 15UL) != 0UL || errno != 0) {
        return -1;
    }

    tls_initialized = expected_initialized;
    tls_zero = expected_zero;
    tls_cookie = expected_cookie;
    errno = ERANGE;
    thrd_yield();
    if (errno != ERANGE) {
        return -2;
    }

    if (mtx_lock(&tls_gate) != thrd_success ||
        mtx_unlock(&tls_gate) != thrd_success) {
        return -3;
    }
    thrd_yield();

    if (tls_initialized != expected_initialized ||
        tls_zero != expected_zero || tls_cookie != expected_cookie ||
        errno != ERANGE) {
        return -4;
    }
    return 300 + arg->id;
}

int main(void)
{
    static const char marker[] = "native-tls-ok";
    struct tls_worker_arg args[TLS_WORKERS];
    thrd_t workers[TLS_WORKERS];
    int index;

    if (tls_initialized != 41 || tls_zero != 0 ||
        tls_cookie != 0x1122334455667788UL ||
        ((unsigned long)&tls_initialized & 15UL) != 0UL) {
        return 1;
    }

    tls_initialized = 51;
    tls_zero = 61;
    tls_cookie = 0x5566778899aabbccUL;
    errno = EIO;
    if (mtx_init(&tls_gate, mtx_plain) != thrd_success ||
        mtx_lock(&tls_gate) != thrd_success) {
        return 2;
    }

    for (index = 0; index < TLS_WORKERS; ++index) {
        args[index].id = index;
        if (thrd_create(&workers[index], tls_worker, &args[index]) !=
                thrd_success ||
            errno != EIO) {
            return 3;
        }
    }

    thrd_yield();
    if (errno != EIO || tls_initialized != 51 || tls_zero != 61 ||
        tls_cookie != 0x5566778899aabbccUL ||
        mtx_unlock(&tls_gate) != thrd_success) {
        return 4;
    }

    for (index = 0; index < TLS_WORKERS; ++index) {
        int result;

        if (thrd_join(workers[index], &result) != thrd_success ||
            result != 300 + index || errno != EIO) {
            return 5;
        }
    }
    mtx_destroy(&tls_gate);

    if (tls_initialized != 51 || tls_zero != 61 ||
        tls_cookie != 0x5566778899aabbccUL || errno != EIO) {
        return 6;
    }
    if (mini_sys_write(1, marker, sizeof(marker) - 1U) !=
        (long)(sizeof(marker) - 1U)) {
        return 7;
    }
    return 0;
}
