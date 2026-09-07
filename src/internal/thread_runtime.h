#ifndef MINI_LIBC_INTERNAL_THREAD_RUNTIME_H
#define MINI_LIBC_INTERNAL_THREAD_RUNTIME_H

#define MINI_TSS_MAX_KEYS 32U
#define MINI_NATIVE_TLS_STORAGE_SIZE 4096U
#define MINI_NATIVE_TLS_MAX_ALIGNMENT 16U

struct mini_thread_tcb {
    _Alignas(MINI_NATIVE_TLS_MAX_ALIGNMENT)
        unsigned char native_tls[MINI_NATIVE_TLS_STORAGE_SIZE];
    struct mini_thread_tcb *self;
    void *control;
    int errno_value;
    unsigned int reserved;
    void *tss_values[MINI_TSS_MAX_KEYS];
    unsigned int tss_generations[MINI_TSS_MAX_KEYS];
};

void __mini_thread_runtime_init_main(long *initial_stack);
void *__mini_thread_prepare_tcb(struct mini_thread_tcb *tcb);
struct mini_thread_tcb *__mini_thread_current_tcb(void);
void __mini_errno_set_provider(int *(*provider)(void));
void __mini_tss_run_destructors(void);

#endif
