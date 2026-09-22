#ifndef MINI_LIBC_INTERNAL_ATFORK_H
#define MINI_LIBC_INTERNAL_ATFORK_H

#define MINI_ATFORK_CAPACITY 8U

struct mini_atfork_handler {
    void (*prepare)(void);
    void (*parent)(void);
    void (*child)(void);
};

unsigned int __mini_atfork_snapshot(
    struct mini_atfork_handler handlers[MINI_ATFORK_CAPACITY]);

#endif
