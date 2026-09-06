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

struct mini_thread_context {
    struct mini_thread_tcb *tcb;
    void *mapping;
    unsigned long mapping_size;
};

void __mini_thread_runtime_init_main(char **envp);
int __mini_thread_context_init(struct mini_thread_context *context,
                               struct mini_thread_tcb *fallback,
                               void *control);
int __mini_thread_context_destroy(struct mini_thread_context *context);
struct mini_thread_tcb *__mini_thread_current_tcb(void);
void __mini_errno_set_provider(int *(*provider)(void));
void __mini_tss_run_destructors(void);

#endif
