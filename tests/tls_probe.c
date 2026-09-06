#include <errno.h>
#include <mini/syscall.h>
#include <threads.h>

#define TLS_WORKERS 2

struct tls_worker_arg {
    int id;
};

static thread_local int tls_initialized = 7;
static thread_local int tls_zero;
static thread_local unsigned long tls_marker = 0x12345678UL;
static void *worker_addresses[TLS_WORKERS];

static int bump_local(void)
{
    static thread_local int local_value = 3;

    return ++local_value;
}

static int tls_worker(void *opaque)
{
    struct tls_worker_arg *arg = (struct tls_worker_arg *)opaque;
    int id = arg->id;
    int expected_errno = 70 + id;

    if (id < 0 || id >= TLS_WORKERS || tls_initialized != 7 ||
        tls_zero != 0 || tls_marker != 0x12345678UL || bump_local() != 4) {
        return 10 + id;
    }

    worker_addresses[id] = (void *)&tls_initialized;
    tls_initialized = 100 + id;
    tls_zero = 200 + id;
    tls_marker = 0x9000UL + (unsigned long)id;
    errno = expected_errno;
    thrd_yield();
    if (errno != expected_errno || tls_initialized != 100 + id ||
        tls_zero != 200 + id ||
        tls_marker != 0x9000UL + (unsigned long)id || bump_local() != 5) {
        return 20 + id;
    }
    return 0;
}

int main(void)
{
    static const char marker[] = "tls-ok\n";
    struct tls_worker_arg args[TLS_WORKERS] = {{0}, {1}};
    thrd_t threads[TLS_WORKERS];
    void *main_address = (void *)&tls_initialized;
    int i;

    if (tls_initialized != 7 || tls_zero != 0 ||
        tls_marker != 0x12345678UL || bump_local() != 4) {
        return 1;
    }

    tls_initialized = 41;
    tls_zero = 42;
    tls_marker = 0xabcdefUL;
    errno = EIO;
    thrd_yield();
    if (errno != EIO) {
        return 2;
    }

    for (i = 0; i < TLS_WORKERS; ++i) {
        if (thrd_create(&threads[i], tls_worker, &args[i]) != thrd_success) {
            return 3;
        }
    }
    for (i = 0; i < TLS_WORKERS; ++i) {
        int result;

        if (thrd_join(threads[i], &result) != thrd_success || result != 0) {
            return 4;
        }
    }

    if (worker_addresses[0] == (void *)0 ||
        worker_addresses[1] == (void *)0 ||
        worker_addresses[0] == worker_addresses[1] ||
        worker_addresses[0] == main_address ||
        worker_addresses[1] == main_address || tls_initialized != 41 ||
        tls_zero != 42 || tls_marker != 0xabcdefUL || bump_local() != 5 ||
        errno != EIO) {
        return 5;
    }

    if (mini_sys_write(1, marker, sizeof(marker) - 1U) !=
        (long)(sizeof(marker) - 1U)) {
        return 6;
    }
    return 0;
}
