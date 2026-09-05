#include <errno.h>
#include <mini/syscall.h>
#include <stdlib.h>
#include <threads.h>

#include "../internal/thread_runtime.h"

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

#define MINI_THREAD_JOINABLE 0
#define MINI_THREAD_JOINING 1
#define MINI_THREAD_DETACHED 2
#define MINI_THREAD_REAPING 3

struct mini_thread_control {
    int tid;
    volatile int clear_tid;
    volatile int result;
    int lifecycle;
    int published;
    void *stack;
    unsigned long stack_size;
    thrd_start_t start;
    void *arg;
    struct mini_thread_control *next;
    struct mini_thread_tcb tcb;
};

struct mini_futex_timeout {
    long tv_sec;
    long tv_nsec;
};

static struct mini_thread_control *mini_thread_controls;
static volatile int mini_thread_registry_lock_word;
static volatile int mini_reaper_event;
static int mini_reaper_started;
static struct mini_thread_control mini_reaper_control;
static const struct mini_futex_timeout mini_reaper_poll = {0L, 10000000L};

extern int __mini_atomic_exchange_int(volatile int *value, int replacement);
extern long __mini_clone_thread(struct mini_thread_control *control,
                                void *stack_top, unsigned long flags,
                                volatile int *child_tid, void *tls);

static int raw_failed(long value)
{
    return value < 0L && value >= -4095L;
}

static void registry_lock(void)
{
    while (__mini_atomic_exchange_int(&mini_thread_registry_lock_word, 1) != 0) {
        (void)mini_sys_futex(&mini_thread_registry_lock_word, MINI_FUTEX_WAIT, 1,
                             (const void *)0, (volatile int *)0, 0);
    }
}

static void registry_unlock(void)
{
    if (__mini_atomic_exchange_int(&mini_thread_registry_lock_word, 0) != 0) {
        (void)mini_sys_futex(&mini_thread_registry_lock_word, MINI_FUTEX_WAKE, 1,
                             (const void *)0, (volatile int *)0, 0);
    }
}

static struct mini_thread_control *find_control_locked(thrd_t thread)
{
    struct mini_thread_control *control = mini_thread_controls;

    while (control != (struct mini_thread_control *)0) {
        if (control->published &&
            (thrd_t)(unsigned int)control->tid == thread) {
            return control;
        }
        control = control->next;
    }
    return (struct mini_thread_control *)0;
}

static void insert_control_locked(struct mini_thread_control *control)
{
    control->next = mini_thread_controls;
    mini_thread_controls = control;
}

static void remove_control_locked(struct mini_thread_control *control)
{
    struct mini_thread_control **link = &mini_thread_controls;

    while (*link != (struct mini_thread_control *)0) {
        if (*link == control) {
            *link = control->next;
            control->next = (struct mini_thread_control *)0;
            return;
        }
        link = &(*link)->next;
    }
}

static struct mini_thread_control *find_reapable_locked(void)
{
    struct mini_thread_control *control = mini_thread_controls;

    while (control != (struct mini_thread_control *)0) {
        if (control->published &&
            control->lifecycle == MINI_THREAD_DETACHED &&
            control->clear_tid == 0) {
            control->lifecycle = MINI_THREAD_REAPING;
            return control;
        }
        control = control->next;
    }
    return (struct mini_thread_control *)0;
}

static struct mini_thread_control *find_detached_locked(void)
{
    struct mini_thread_control *control = mini_thread_controls;

    while (control != (struct mini_thread_control *)0) {
        if (control->published &&
            control->lifecycle == MINI_THREAD_DETACHED) {
            return control;
        }
        control = control->next;
    }
    return (struct mini_thread_control *)0;
}

static void notify_reaper_locked(void)
{
    ++mini_reaper_event;
    (void)mini_sys_futex(&mini_reaper_event, MINI_FUTEX_WAKE, 1,
                         (const void *)0, (volatile int *)0, 0);
}

static int reap_claimed_control(struct mini_thread_control *control,
                                int restore_joinable)
{
    long unmap_result = mini_sys_munmap(control->stack, control->stack_size);

    if (raw_failed(unmap_result)) {
        registry_lock();
        control->lifecycle =
            restore_joinable ? MINI_THREAD_JOINABLE : MINI_THREAD_DETACHED;
        if (!restore_joinable) {
            notify_reaper_locked();
        }
        registry_unlock();
        return 0;
    }

    registry_lock();
    remove_control_locked(control);
    registry_unlock();
    free(control);
    return 1;
}

static int reaper_worker(void *opaque)
{
    (void)opaque;

    for (;;) {
        struct mini_thread_control *control;
        int observed_event;
        int observed_tid;

        registry_lock();
        control = find_reapable_locked();
        if (control != (struct mini_thread_control *)0) {
            registry_unlock();
            (void)reap_claimed_control(control, 0);
            continue;
        }

        control = find_detached_locked();
        observed_event = mini_reaper_event;
        observed_tid = control != (struct mini_thread_control *)0
                           ? control->clear_tid
                           : 0;
        registry_unlock();

        if (control != (struct mini_thread_control *)0 && observed_tid != 0) {
            (void)mini_sys_futex(&control->clear_tid, MINI_FUTEX_WAIT,
                                 observed_tid, &mini_reaper_poll,
                                 (volatile int *)0, 0);
        } else {
            (void)mini_sys_futex(&mini_reaper_event, MINI_FUTEX_WAIT,
                                 observed_event, (const void *)0,
                                 (volatile int *)0, 0);
        }
    }
}

static int ensure_reaper_locked(void)
{
    long mapping;
    long clone_result;
    char *stack_top;

    if (mini_reaper_started) {
        return 1;
    }

    mapping = mini_sys_mmap((void *)0, MINI_THREAD_STACK_SIZE,
                            MINI_PROT_READ | MINI_PROT_WRITE,
                            MINI_MAP_PRIVATE | MINI_MAP_ANONYMOUS, -1, 0L);
    if (raw_failed(mapping)) {
        return 0;
    }

    mini_reaper_control.tid = 0;
    mini_reaper_control.clear_tid = -1;
    mini_reaper_control.result = 0;
    mini_reaper_control.lifecycle = MINI_THREAD_DETACHED;
    mini_reaper_control.published = 1;
    mini_reaper_control.stack = (void *)mapping;
    mini_reaper_control.stack_size = MINI_THREAD_STACK_SIZE;
    mini_reaper_control.start = reaper_worker;
    mini_reaper_control.arg = (void *)0;
    mini_reaper_control.next = (struct mini_thread_control *)0;
    mini_reaper_control.tcb.self = &mini_reaper_control.tcb;
    mini_reaper_control.tcb.control = &mini_reaper_control;
    mini_reaper_control.tcb.errno_value = 0;
    mini_reaper_control.tcb.reserved = 0U;

    stack_top = (char *)mini_reaper_control.stack +
                mini_reaper_control.stack_size;
    clone_result = __mini_clone_thread(&mini_reaper_control, stack_top,
                                       MINI_THREAD_CLONE_FLAGS,
                                       &mini_reaper_control.clear_tid,
                                       &mini_reaper_control.tcb);
    if (raw_failed(clone_result)) {
        (void)mini_sys_munmap(mini_reaper_control.stack,
                              mini_reaper_control.stack_size);
        mini_reaper_control.stack = (void *)0;
        mini_reaper_control.stack_size = 0UL;
        mini_reaper_control.start = (thrd_start_t)0;
        mini_reaper_control.tcb.self = (struct mini_thread_tcb *)0;
        mini_reaper_control.tcb.control = (void *)0;
        return 0;
    }

    mini_reaper_control.tid = (int)clone_result;
    mini_reaper_started = 1;
    return 1;
}

int thrd_create(thrd_t *thr, thrd_start_t func, void *arg)
{
    struct mini_thread_control *control;
    long mapping;
    long clone_result;
    char *stack_top;
    int saved_errno = errno;

    if (thr == (thrd_t *)0 || func == (thrd_start_t)0) {
        errno = saved_errno;
        return thrd_error;
    }

    control = (struct mini_thread_control *)malloc(sizeof(*control));
    if (control == (struct mini_thread_control *)0) {
        errno = saved_errno;
        return thrd_nomem;
    }

    mapping = mini_sys_mmap((void *)0, MINI_THREAD_STACK_SIZE,
                            MINI_PROT_READ | MINI_PROT_WRITE,
                            MINI_MAP_PRIVATE | MINI_MAP_ANONYMOUS, -1, 0L);
    if (raw_failed(mapping)) {
        free(control);
        errno = saved_errno;
        return thrd_nomem;
    }

    control->tid = 0;
    control->clear_tid = -1;
    control->result = 0;
    control->lifecycle = MINI_THREAD_JOINABLE;
    control->published = 0;
    control->stack = (void *)mapping;
    control->stack_size = MINI_THREAD_STACK_SIZE;
    control->start = func;
    control->arg = arg;
    control->next = (struct mini_thread_control *)0;
    control->tcb.self = &control->tcb;
    control->tcb.control = control;
    control->tcb.errno_value = 0;
    control->tcb.reserved = 0U;

    registry_lock();
    insert_control_locked(control);
    registry_unlock();

    stack_top = (char *)control->stack + control->stack_size;
    clone_result = __mini_clone_thread(control, stack_top,
                                       MINI_THREAD_CLONE_FLAGS,
                                       &control->clear_tid, &control->tcb);
    if (raw_failed(clone_result)) {
        registry_lock();
        remove_control_locked(control);
        registry_unlock();
        (void)mini_sys_munmap(control->stack, control->stack_size);
        free(control);
        errno = saved_errno;
        return clone_result == MINI_RAW_ENOMEM ? thrd_nomem : thrd_error;
    }

    registry_lock();
    control->tid = (int)clone_result;
    control->published = 1;
    if (control->lifecycle == MINI_THREAD_DETACHED) {
        notify_reaper_locked();
    }
    registry_unlock();

    *thr = (thrd_t)(unsigned long)clone_result;
    errno = saved_errno;
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

int thrd_detach(thrd_t thr)
{
    struct mini_thread_control *control;
    struct mini_thread_control *self_control = (struct mini_thread_control *)0;
    int saved_errno = errno;

    if (thr == (thrd_t)0) {
        errno = saved_errno;
        return thrd_error;
    }

    if (thrd_equal(thr, thrd_current())) {
        struct mini_thread_tcb *tcb = __mini_thread_current_tcb();

        self_control = (struct mini_thread_control *)tcb->control;
    }

    registry_lock();
    control = self_control != (struct mini_thread_control *)0
                  ? self_control
                  : find_control_locked(thr);
    if (control == (struct mini_thread_control *)0 ||
        control == &mini_reaper_control ||
        control->lifecycle != MINI_THREAD_JOINABLE) {
        registry_unlock();
        errno = saved_errno;
        return thrd_error;
    }

    if (control->clear_tid == 0 && control->published) {
        control->lifecycle = MINI_THREAD_REAPING;
        registry_unlock();
        if (!reap_claimed_control(control, 1)) {
            errno = saved_errno;
            return thrd_error;
        }
        errno = saved_errno;
        return thrd_success;
    }

    if (!ensure_reaper_locked()) {
        registry_unlock();
        errno = saved_errno;
        return thrd_error;
    }
    control->lifecycle = MINI_THREAD_DETACHED;
    if (control->published) {
        notify_reaper_locked();
    }
    registry_unlock();
    errno = saved_errno;
    return thrd_success;
}

int thrd_join(thrd_t thr, int *res)
{
    struct mini_thread_control *control;
    long unmap_result;
    int saved_errno = errno;

    if (thr == (thrd_t)0 || thrd_equal(thr, thrd_current())) {
        errno = saved_errno;
        return thrd_error;
    }

    registry_lock();
    control = find_control_locked(thr);
    if (control == (struct mini_thread_control *)0 ||
        control->lifecycle != MINI_THREAD_JOINABLE) {
        registry_unlock();
        errno = saved_errno;
        return thrd_error;
    }
    control->lifecycle = MINI_THREAD_JOINING;
    registry_unlock();

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
            registry_lock();
            control->lifecycle = MINI_THREAD_JOINABLE;
            registry_unlock();
            errno = saved_errno;
            return thrd_error;
        }
    }

    unmap_result = mini_sys_munmap(control->stack, control->stack_size);
    if (raw_failed(unmap_result)) {
        registry_lock();
        control->lifecycle = MINI_THREAD_JOINABLE;
        registry_unlock();
        errno = saved_errno;
        return thrd_error;
    }

    if (res != (int *)0) {
        *res = control->result;
    }
    registry_lock();
    remove_control_locked(control);
    registry_unlock();
    free(control);
    errno = saved_errno;
    return thrd_success;
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
