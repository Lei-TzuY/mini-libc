#include <errno.h>
#include <mini/syscall.h>
#include <threads.h>

#define TINY_TLS_WORKERS 4

_Alignas(16) thread_local int tiny_tls_initialized = 17;
thread_local int tiny_tls_zero;
thread_local unsigned long tiny_tls_cookie = 0x12345678UL;

static mtx_t tiny_tls_gate;

struct tiny_tls_arg {
    int id;
};

static int tiny_tls_worker(void *opaque)
{
    struct tiny_tls_arg *arg = (struct tiny_tls_arg *)opaque;
    int expected = 80 + arg->id;

    if (tiny_tls_initialized != 17 || tiny_tls_zero != 0 ||
        tiny_tls_cookie != 0x12345678UL ||
        ((unsigned long)&tiny_tls_initialized & 15UL) != 0UL || errno != 0) {
        return -1;
    }

    tiny_tls_initialized = expected;
    tiny_tls_zero = expected + 20;
    tiny_tls_cookie = 0x90000000UL + (unsigned long)arg->id;
    errno = ERANGE;
    thrd_yield();
    if (errno != ERANGE || mtx_lock(&tiny_tls_gate) != thrd_success ||
        mtx_unlock(&tiny_tls_gate) != thrd_success) {
        return -2;
    }
    thrd_yield();

    if (tiny_tls_initialized != expected || tiny_tls_zero != expected + 20 ||
        tiny_tls_cookie != 0x90000000UL + (unsigned long)arg->id ||
        errno != ERANGE) {
        return -3;
    }
    return 120 + arg->id;
}

int main(void)
{
    static const char marker[] = "tiny-native-tls-ok";
    struct tiny_tls_arg args[TINY_TLS_WORKERS];
    thrd_t workers[TINY_TLS_WORKERS];
    int index;

    if (tiny_tls_initialized != 17 || tiny_tls_zero != 0 ||
        tiny_tls_cookie != 0x12345678UL ||
        ((unsigned long)&tiny_tls_initialized & 15UL) != 0UL) {
        return 1;
    }

    tiny_tls_initialized = 27;
    tiny_tls_zero = 37;
    tiny_tls_cookie = 0x87654321UL;
    errno = EIO;
    if (mtx_init(&tiny_tls_gate, mtx_plain) != thrd_success ||
        mtx_lock(&tiny_tls_gate) != thrd_success) {
        return 2;
    }

    for (index = 0; index < TINY_TLS_WORKERS; ++index) {
        args[index].id = index;
        if (thrd_create(&workers[index], tiny_tls_worker, &args[index]) !=
                thrd_success ||
            errno != EIO) {
            return 3;
        }
    }

    thrd_yield();
    if (errno != EIO || tiny_tls_initialized != 27 || tiny_tls_zero != 37 ||
        tiny_tls_cookie != 0x87654321UL ||
        mtx_unlock(&tiny_tls_gate) != thrd_success) {
        return 4;
    }

    for (index = 0; index < TINY_TLS_WORKERS; ++index) {
        int result;

        if (thrd_join(workers[index], &result) != thrd_success ||
            result != 120 + index || errno != EIO) {
            return 5;
        }
    }
    mtx_destroy(&tiny_tls_gate);

    if (tiny_tls_initialized != 27 || tiny_tls_zero != 37 ||
        tiny_tls_cookie != 0x87654321UL || errno != EIO) {
        return 6;
    }
    if (mini_sys_write(1, marker, sizeof(marker) - 1U) !=
        (long)(sizeof(marker) - 1U)) {
        return 7;
    }
    return 0;
}
