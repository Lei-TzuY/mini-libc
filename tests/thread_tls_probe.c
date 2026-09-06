#include <errno.h>
#include <mini/syscall.h>
#include <threads.h>

static thread_local int tls_initialized = 17;
static thread_local int tls_zero;

struct tls_worker_arg {
    int value;
    int initial_initialized;
    int initial_zero;
    int final_initialized;
    int final_zero;
    int *address;
};

static int tls_worker(void *opaque)
{
    struct tls_worker_arg *arg = (struct tls_worker_arg *)opaque;

    arg->initial_initialized = tls_initialized;
    arg->initial_zero = tls_zero;
    arg->address = &tls_initialized;
    if (tls_initialized != 17 || tls_zero != 0) {
        return -1;
    }

    tls_initialized = arg->value;
    tls_zero = arg->value + 100;
    errno = EIO;
    thrd_yield();
    if (errno != EIO) {
        return -2;
    }

    arg->final_initialized = tls_initialized;
    arg->final_zero = tls_zero;
    return 0;
}

int main(void)
{
    static const char marker[] = "thread-tls-ok";
    struct tls_worker_arg first = {31, 0, 0, 0, 0, (int *)0};
    struct tls_worker_arg second = {47, 0, 0, 0, 0, (int *)0};
    thrd_t first_thread;
    thrd_t second_thread;
    int first_result;
    int second_result;
    int *main_address = &tls_initialized;

    if (tls_initialized != 17 || tls_zero != 0) {
        return 1;
    }
    tls_initialized = 23;
    tls_zero = 42;

    errno = ERANGE;
    thrd_yield();
    if (errno != ERANGE) {
        return 2;
    }

    if (thrd_create(&first_thread, tls_worker, &first) != thrd_success ||
        thrd_create(&second_thread, tls_worker, &second) != thrd_success) {
        return 3;
    }
    if (thrd_join(first_thread, &first_result) != thrd_success ||
        thrd_join(second_thread, &second_result) != thrd_success ||
        first_result != 0 || second_result != 0) {
        return 4;
    }

    if (first.initial_initialized != 17 || first.initial_zero != 0 ||
        second.initial_initialized != 17 || second.initial_zero != 0 ||
        first.final_initialized != first.value ||
        first.final_zero != first.value + 100 ||
        second.final_initialized != second.value ||
        second.final_zero != second.value + 100) {
        return 5;
    }
    if (first.address == (int *)0 || second.address == (int *)0 ||
        first.address == second.address || first.address == main_address ||
        second.address == main_address) {
        return 6;
    }
    if (tls_initialized != 23 || tls_zero != 42 || errno != ERANGE) {
        return 7;
    }

    if (mini_sys_write(1, marker, sizeof(marker) - 1U) !=
        (long)(sizeof(marker) - 1U)) {
        return 8;
    }
    return 0;
}
