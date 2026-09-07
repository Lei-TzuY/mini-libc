#ifndef MINI_LIBC_THREADS_H
#define MINI_LIBC_THREADS_H

#include <stdatomic.h>
#include <time.h>

#ifndef thread_local
#define thread_local _Thread_local
#endif

typedef unsigned long thrd_t;
typedef int (*thrd_start_t)(void *);
typedef unsigned int tss_t;
typedef void (*tss_dtor_t)(void *);

typedef struct {
    atomic_int __state;
} once_flag;

#define ONCE_FLAG_INIT {ATOMIC_VAR_INIT(0)}
#define TSS_DTOR_ITERATIONS 4

typedef struct {
    atomic_int __state;
    int __type;
    atomic_ulong __owner;
    atomic_int __depth;
} mtx_t;

typedef struct {
    atomic_int __sequence;
} cnd_t;

_Static_assert(sizeof(atomic_int) == sizeof(int),
               "C11 atomic int must preserve futex word size");
_Static_assert(sizeof(atomic_ulong) == sizeof(unsigned long),
               "C11 atomic ulong must preserve owner word size");
_Static_assert(sizeof(once_flag) == sizeof(int),
               "once_flag ABI must remain one futex word");
_Static_assert(sizeof(cnd_t) == sizeof(int),
               "cnd_t ABI must remain one futex word");
_Static_assert(sizeof(mtx_t) == 24,
               "mtx_t ABI must remain stable on x86-64");

enum {
    thrd_success = 0,
    thrd_nomem = 1,
    thrd_timedout = 2,
    thrd_busy = 3,
    thrd_error = 4
};

enum {
    mtx_plain = 0,
    mtx_recursive = 1,
    mtx_timed = 2
};

void call_once(once_flag *flag, void (*func)(void));

int thrd_create(thrd_t *thr, thrd_start_t func, void *arg);
int thrd_detach(thrd_t thr);
int thrd_join(thrd_t thr, int *res);
thrd_t thrd_current(void);
int thrd_equal(thrd_t lhs, thrd_t rhs);
int thrd_sleep(const struct timespec *duration, struct timespec *remaining);
void thrd_yield(void);
_Noreturn void thrd_exit(int res);

int tss_create(tss_t *key, tss_dtor_t dtor);
void tss_delete(tss_t key);
void *tss_get(tss_t key);
int tss_set(tss_t key, void *value);

int mtx_init(mtx_t *mtx, int type);
int mtx_lock(mtx_t *mtx);
int mtx_trylock(mtx_t *mtx);
int mtx_timedlock(mtx_t *restrict mtx,
                  const struct timespec *restrict time_point);
int mtx_unlock(mtx_t *mtx);
void mtx_destroy(mtx_t *mtx);

int cnd_init(cnd_t *cond);
int cnd_signal(cnd_t *cond);
int cnd_broadcast(cnd_t *cond);
int cnd_wait(cnd_t *cond, mtx_t *mtx);
int cnd_timedwait(cnd_t *restrict cond, mtx_t *restrict mtx,
                  const struct timespec *restrict time_point);
void cnd_destroy(cnd_t *cond);

#endif
