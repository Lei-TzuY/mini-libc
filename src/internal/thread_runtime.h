#ifndef MINI_LIBC_INTERNAL_THREAD_RUNTIME_H
#define MINI_LIBC_INTERNAL_THREAD_RUNTIME_H

#define MINI_TSS_MAX_KEYS 32U

struct mini_thread_tcb {
    struct mini_thread_tcb *self;
    void *control;
    int errno_value;
    unsigned int reserved;
    void *tss_values[MINI_TSS_MAX_KEYS];
    unsigned int tss_generations[MINI_TSS_MAX_KEYS];
};

void __mini_thread_runtime_init_main(void);
struct mini_thread_tcb *__mini_thread_current_tcb(void);
void __mini_errno_set_provider(int *(*provider)(void));
void __mini_tss_run_destructors(void);

#endif
