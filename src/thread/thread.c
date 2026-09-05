#include <errno.h>
#include <mini/syscall.h>
#include <threads.h>

#include "../internal/thread_runtime.h"

#define MINI_THREAD_MAX 16U
#define MINI_THREAD_STACK_SIZE (1024UL * 1024UL)

#define MINI_PROT_READ 1
#define MINI_PROT_WRITE 2
#define MINI_MAP_PRIVATE 2
#define MINI_MAP_ANONYMOUS 32

#define MINI_CLONE_VM 0x00000100UL
#define MINI_CLONE_FS 0x00000200UL
#define MINI_CLONE_FILES 0x00000400UL
#define MINI_CLONE_SIGHAND 0x00000800UL
#define MINI_CLONE_THREAD 0x00010000UL
#define MINI_CLONE_SYSVSEM 0x00040000UL
#define MINI_CLONE_SETTLS 0x00080000UL
#define MINI_CLONE_CHILD_CLEARTID 0x00200000UL
#define MINI_CLONE_CHILD_SETTID 0x01000000UL
#define MINI_THREAD_CLONE_FLAGS                                                \
    (MINI_CLONE_VM | MINI_CLONE_FS | MINI_CLONE_FILES | MINI_CLONE_SIGHAND | \
     MINI_CLONE_THREAD | MINI_CLONE_SYSVSEM | MINI_CLONE_SETTLS |             \
     MINI_CLONE_CHILD_CLEARTID | MINI_CLONE_CHILD_SETTID)

#define MINI_FUTEX_WAIT 0
#define MINI_FUTEX_WAKE 1
#define MINI_RAW_EINTR (-4L)
#define MINI_RAW_EAGAIN (-11L)
#define MINI_RAW_ENOMEM (-12L)

struct mini_thread_control {
    volatile int in_use;
    int tid;
    volatile int clear_tid;
    volatile int result;
    void *stack;
    unsigned long stack_size;
    thrd_start_t start;
    void *arg;
    struct mini_thread_tcb tcb;
};

static struct mini_thread_control mini_thread_controls[MINI_THREAD_MAX];

extern int __mini_atomic_exchange_int(volatile int *value, int replacement);
extern long __mini_clone_thread(struct mini_thread_control *control,
                                void *stack_top, unsigned long flags,
                                volatile int *child_tid, void *tls);

static int raw_failed(long value)
{
    return value < 0L && value >= -4095L;
}

static struct mini_thread_control *reserve_control(void)
{
    unsigned int i;

    for (i = 0; i < MINI_THREAD_MAX; ++i) {
        if (__mini_atomic_exchange_int(&mini_thread_controls[i].in_use, 1) == 0) {
            return &mini_thread_controls[i];
        }
    }
    return (struct mini_thread_control *)0;
}

static void release_control(struct mini_thread_control *control)
{
    control->tid = 0;
    control->clear_tid = 0;
    control->result = 0;
    control->stack = (void *)0;
    control->stack_size = 0UL;
    control->start = (thrd_start_t)0;
    control->arg = (void *)0;
    control->tcb.self = (struct mini_thread_tcb *)0;
    control->tcb.control = (void *)0;
    control->tcb.errno_value = 0;
    control->tcb.reserved = 0U;
    (void)__mini_atomic_exchange_int(&control->in_use, 0);
}

static struct mini_thread_control *find_control(thrd_t thread)
{
    unsigned int i;

    for (i = 0; i < MINI_THREAD_MAX; ++i) {
        if (mini_thread_controls[i].in_use != 0 &&
            (thrd_t)(unsigned int)mini_thread_controls[i].tid == thread) {
            return &mini_thread_controls[i];
        }
    }
    return (struct mini_thread_control *)0;
}

int thrd_create(thrd_t *thr, thrd_start_t func, void *arg)
{
    struct mini_thread_control *control;
    long mapping;
    long clone_result;
    char *stack_top;

    if (thr == (thrd_t *)0 || func == (thrd_start_t)0) {
        return thrd_error;
    }

    control = reserve_control();
    if (control == (struct mini_thread_control *)0) {
        return thrd_nomem;
    }

    mapping = mini_sys_mmap((void *)0, MINI_THREAD_STACK_SIZE,
                            MINI_PROT_READ | MINI_PROT_WRITE,
                            MINI_MAP_PRIVATE | MINI_MAP_ANONYMOUS, -1, 0L);
    if (raw_failed(mapping)) {
        release_control(control);
        return thrd_nomem;
    }

    control->tid = 0;
    control->clear_tid = -1;
    control->result = 0;
    control->stack = (void *)mapping;
    control->stack_size = MINI_THREAD_STACK_SIZE;
    control->start = func;
    control->arg = arg;
    control->tcb.self = &control->tcb;
    control->tcb.control = control;
    control->tcb.errno_value = 0;
    control->tcb.reserved = 0U;

    stack_top = (char *)control->stack + control->stack_size;
    clone_result = __mini_clone_thread(control, stack_top,
                                       MINI_THREAD_CLONE_FLAGS,
                                       &control->clear_tid, &control->tcb);
    if (raw_failed(clone_result)) {
        (void)mini_sys_munmap(control->stack, control->stack_size);
        release_control(control);
        return clone_result == MINI_RAW_ENOMEM ? thrd_nomem : thrd_error;
    }

    control->tid = (int)clone_result;
    *thr = (thrd_t)(unsigned int)control->tid;
    return thrd_success;
}

void __mini_thread_run(struct mini_thread_control *control)
{
    int result = control->start(control->arg);

    thrd_exit(result);
}

_Noreturn void thrd_exit(int res)
{
    struct mini_thread_tcb *tcb = __mini_thread_current_tcb();
    struct mini_thread_control *control =
        (struct mini_thread_control *)tcb->control;

    if (control != (struct mini_thread_control *)0) {
        control->result = res;
    }
    mini_sys_exit(0);
}

thrd_t thrd_current(void)
{
    long tid = mini_sys_gettid();

    return tid > 0L ? (thrd_t)(unsigned long)tid : (thrd_t)0;
}

int thrd_equal(thrd_t lhs, thrd_t rhs)
{
    return lhs == rhs;
}

int thrd_join(thrd_t thr, int *res)
{
    struct mini_thread_control *control;
    long unmap_result;

    if (thr == (thrd_t)0 || thrd_equal(thr, thrd_current())) {
        return thrd_error;
    }

    control = find_control(thr);
    if (control == (struct mini_thread_control *)0) {
        return thrd_error;
    }

    for (;;) {
        int observed = control->clear_tid;
        long wait_result;

        if (observed == 0) {
            break;
        }
        wait_result = mini_sys_futex(&control->clear_tid, MINI_FUTEX_WAIT,
                                     observed, (const void *)0,
                                     (volatile int *)0, 0);
        if (wait_result < 0L && wait_result != MINI_RAW_EINTR &&
            wait_result != MINI_RAW_EAGAIN) {
            return thrd_error;
        }
    }

    if (res != (int *)0) {
        *res = control->result;
    }
    unmap_result = mini_sys_munmap(control->stack, control->stack_size);
    release_control(control);
    return raw_failed(unmap_result) ? thrd_error : thrd_success;
}

int mtx_init(mtx_t *mtx, int type)
{
    if (mtx == (mtx_t *)0 || type != mtx_plain) {
        return thrd_error;
    }
    mtx->__state = 0;
    return thrd_success;
}

int mtx_trylock(mtx_t *mtx)
{
    if (mtx == (mtx_t *)0) {
        return thrd_error;
    }
    return __mini_atomic_exchange_int((volatile int *)&mtx->__state, 1) == 0
               ? thrd_success
               : thrd_busy;
}

int mtx_lock(mtx_t *mtx)
{
    if (mtx == (mtx_t *)0) {
        return thrd_error;
    }

    while (__mini_atomic_exchange_int((volatile int *)&mtx->__state, 1) != 0) {
        long wait_result = mini_sys_futex((volatile int *)&mtx->__state,
                                          MINI_FUTEX_WAIT, 1,
                                          (const void *)0,
                                          (volatile int *)0, 0);
        if (wait_result < 0L && wait_result != MINI_RAW_EINTR &&
            wait_result != MINI_RAW_EAGAIN) {
            return thrd_error;
        }
    }
    return thrd_success;
}

int mtx_unlock(mtx_t *mtx)
{
    int previous;

    if (mtx == (mtx_t *)0) {
        return thrd_error;
    }
    previous = __mini_atomic_exchange_int((volatile int *)&mtx->__state, 0);
    if (previous == 0) {
        return thrd_error;
    }
    (void)mini_sys_futex((volatile int *)&mtx->__state, MINI_FUTEX_WAKE, 1,
                         (const void *)0, (volatile int *)0, 0);
    return thrd_success;
}

void mtx_destroy(mtx_t *mtx)
{
    if (mtx != (mtx_t *)0) {
        (void)__mini_atomic_exchange_int((volatile int *)&mtx->__state, 0);
    }
}
