#ifndef MINI_LIBC_THREADS_H
#define MINI_LIBC_THREADS_H

#include <time.h>

typedef unsigned long thrd_t;
typedef int (*thrd_start_t)(void *);

typedef struct {
    int __state;
    int __type;
    unsigned long __owner;
    int __depth;
} mtx_t;

typedef struct {
    int __sequence;
} cnd_t;

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

int thrd_create(thrd_t *thr, thrd_start_t func, void *arg);
int thrd_detach(thrd_t thr);
int thrd_join(thrd_t thr, int *res);
thrd_t thrd_current(void);
int thrd_equal(thrd_t lhs, thrd_t rhs);
int thrd_sleep(const struct timespec *duration, struct timespec *remaining);
_Noreturn void thrd_exit(int res);

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
