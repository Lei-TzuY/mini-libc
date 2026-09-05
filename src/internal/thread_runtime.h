#ifndef MINI_LIBC_INTERNAL_THREAD_RUNTIME_H
#define MINI_LIBC_INTERNAL_THREAD_RUNTIME_H

struct mini_thread_tcb {
    struct mini_thread_tcb *self;
    void *control;
    int errno_value;
    unsigned int reserved;
};

void __mini_thread_runtime_init_main(void);
struct mini_thread_tcb *__mini_thread_current_tcb(void);
void __mini_errno_set_provider(int *(*provider)(void));

#endif
