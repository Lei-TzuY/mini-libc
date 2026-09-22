#include <errno.h>
#include <pthread.h>

#include "../internal/atfork.h"
#include "../internal/futex_lock.h"

static struct mini_futex_lock atfork_lock = MINI_FUTEX_LOCK_INIT;
static struct mini_atfork_handler atfork_handlers[MINI_ATFORK_CAPACITY];
static unsigned int atfork_count;

int pthread_atfork(void (*prepare)(void),
                   void (*parent)(void),
                   void (*child)(void))
{
    int saved_errno = errno;
    unsigned int index;

    mini_futex_lock_acquire(&atfork_lock);
    if (atfork_count == MINI_ATFORK_CAPACITY) {
        mini_futex_lock_release(&atfork_lock);
        errno = saved_errno;
        return ENOMEM;
    }

    index = atfork_count;
    atfork_handlers[index].prepare = prepare;
    atfork_handlers[index].parent = parent;
    atfork_handlers[index].child = child;
    atfork_count = index + 1U;
    mini_futex_lock_release(&atfork_lock);

    errno = saved_errno;
    return 0;
}

unsigned int __mini_atfork_snapshot(
    struct mini_atfork_handler handlers[MINI_ATFORK_CAPACITY])
{
    unsigned int index;
    unsigned int count;

    mini_futex_lock_acquire(&atfork_lock);
    count = atfork_count;
    for (index = 0U; index < count; ++index) {
        handlers[index] = atfork_handlers[index];
    }
    mini_futex_lock_release(&atfork_lock);
    return count;
}
