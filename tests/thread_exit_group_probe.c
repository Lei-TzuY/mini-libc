#include <mini/syscall.h>
#include <stdlib.h>
#include <threads.h>

#define MINI_FUTEX_WAIT 0

struct mini_wait_timeout {
    long tv_sec;
    long tv_nsec;
};

static volatile int child_ready;
static volatile int wait_word;

static int survivor(void *opaque)
{
    static const char marker[] = "survived";
    struct mini_wait_timeout timeout = {1L, 0L};

    (void)opaque;
    child_ready = 1;
    (void)mini_sys_futex(&wait_word, MINI_FUTEX_WAIT, 0, &timeout,
                         (volatile int *)0, 0);
    (void)mini_sys_write(1, marker, sizeof(marker) - 1U);
    return 0;
}

int main(void)
{
    thrd_t worker;

    if (thrd_create(&worker, survivor, (void *)0) != thrd_success) {
        return 2;
    }
    while (child_ready == 0) {
    }

    _Exit(37);
}
