#include <mini/syscall.h>

#include "../internal/thread_runtime.h"

#define MINI_ARCH_SET_FS 0x1002
#define MINI_THREAD_INIT_FAILURE 127
#define MINI_PROT_READ 1
#define MINI_PROT_WRITE 2
#define MINI_MAP_PRIVATE 2
#define MINI_MAP_ANONYMOUS 32
#define MINI_TCB_ALIGNMENT 8UL

#define MINI_AT_NULL 0UL
#define MINI_AT_PHDR 3UL
#define MINI_AT_PHENT 4UL
#define MINI_AT_PHNUM 5UL
#define MINI_PT_TLS 7U

struct mini_auxv_entry {
    unsigned long type;
    unsigned long value;
};

struct mini_elf64_phdr {
    unsigned int type;
    unsigned int flags;
    unsigned long offset;
    unsigned long virtual_address;
    unsigned long physical_address;
    unsigned long file_size;
    unsigned long memory_size;
    unsigned long alignment;
};

struct mini_tls_template {
    const unsigned char *image;
    unsigned long file_size;
    unsigned long memory_size;
    unsigned long block_size;
    unsigned long alignment;
    int present;
};

static struct mini_tls_template mini_tls_template;
static struct mini_thread_tcb mini_main_tcb;
static struct mini_thread_context mini_main_context;

static int raw_failed(long value)
{
    return value < 0L && value >= -4095L;
}

static int power_of_two(unsigned long value)
{
    return value != 0UL && (value & (value - 1UL)) == 0UL;
}

static int add_overflow(unsigned long left, unsigned long right,
                        unsigned long *result)
{
    unsigned long sum = left + right;

    if (sum < left) {
        return 1;
    }
    *result = sum;
    return 0;
}

static int align_up(unsigned long value, unsigned long alignment,
                    unsigned long *result)
{
    unsigned long mask = alignment - 1UL;
    unsigned long adjusted;

    if (!power_of_two(alignment) || add_overflow(value, mask, &adjusted)) {
        return 0;
    }
    *result = adjusted & ~mask;
    return 1;
}

static void initialize_tcb(struct mini_thread_tcb *tcb, void *control)
{
    unsigned int index;

    tcb->self = tcb;
    tcb->control = control;
    tcb->errno_value = 0;
    tcb->reserved = 0U;
    for (index = 0U; index < MINI_TSS_MAX_KEYS; ++index) {
        tcb->tss_values[index] = (void *)0;
        tcb->tss_generations[index] = 0U;
    }
}

static int discover_static_tls(char **envp)
{
    struct mini_auxv_entry *auxv;
    unsigned long phdr_address = 0UL;
    unsigned long phent = 0UL;
    unsigned long phnum = 0UL;
    unsigned long index;

    mini_tls_template.present = 0;
    while (*envp != (char *)0) {
        ++envp;
    }
    auxv = (struct mini_auxv_entry *)(envp + 1);
    while (auxv->type != MINI_AT_NULL) {
        if (auxv->type == MINI_AT_PHDR) {
            phdr_address = auxv->value;
        } else if (auxv->type == MINI_AT_PHENT) {
            phent = auxv->value;
        } else if (auxv->type == MINI_AT_PHNUM) {
            phnum = auxv->value;
        }
        ++auxv;
    }

    if (phdr_address == 0UL || phnum == 0UL) {
        return 1;
    }
    if (phent < (unsigned long)sizeof(struct mini_elf64_phdr)) {
        return 0;
    }

    for (index = 0UL; index < phnum; ++index) {
        const struct mini_elf64_phdr *phdr =
            (const struct mini_elf64_phdr *)(phdr_address + index * phent);
        unsigned long alignment;
        unsigned long block_size;

        if (phdr->type != MINI_PT_TLS) {
            continue;
        }
        if (phdr->file_size > phdr->memory_size) {
            return 0;
        }
        alignment = phdr->alignment == 0UL ? 1UL : phdr->alignment;
        if (!power_of_two(alignment) ||
            !align_up(phdr->memory_size, alignment, &block_size)) {
            return 0;
        }
        if (phdr->file_size != 0UL && phdr->virtual_address == 0UL) {
            return 0;
        }

        mini_tls_template.image =
            (const unsigned char *)phdr->virtual_address;
        mini_tls_template.file_size = phdr->file_size;
        mini_tls_template.memory_size = phdr->memory_size;
        mini_tls_template.block_size = block_size;
        mini_tls_template.alignment = alignment;
        mini_tls_template.present = 1;
        return 1;
    }
    return 1;
}

static int *thread_errno_location(void)
{
    struct mini_thread_tcb *tcb = __mini_thread_current_tcb();

    return &tcb->errno_value;
}

int __mini_thread_context_init(struct mini_thread_context *context,
                               struct mini_thread_tcb *fallback,
                               void *control)
{
    unsigned long alignment;
    unsigned long reserve;
    unsigned long mapping_end;
    unsigned long minimum_tp;
    unsigned long tp;
    unsigned long tls_start;
    unsigned long index;
    long mapping;
    struct mini_thread_tcb *tcb;

    if (!mini_tls_template.present) {
        initialize_tcb(fallback, control);
        context->tcb = fallback;
        context->mapping = (void *)0;
        context->mapping_size = 0UL;
        return 1;
    }

    alignment = mini_tls_template.alignment;
    if (alignment < MINI_TCB_ALIGNMENT) {
        alignment = MINI_TCB_ALIGNMENT;
    }
    if (add_overflow(mini_tls_template.block_size, alignment - 1UL,
                     &reserve) ||
        add_overflow(reserve, (unsigned long)sizeof(struct mini_thread_tcb),
                     &reserve)) {
        return 0;
    }

    mapping = mini_sys_mmap((void *)0, reserve, MINI_PROT_READ | MINI_PROT_WRITE,
                            MINI_MAP_PRIVATE | MINI_MAP_ANONYMOUS, -1, 0L);
    if (raw_failed(mapping)) {
        return 0;
    }
    if (add_overflow((unsigned long)mapping, reserve, &mapping_end) ||
        add_overflow((unsigned long)mapping, mini_tls_template.block_size,
                     &minimum_tp) ||
        !align_up(minimum_tp, alignment, &tp) ||
        tp > mapping_end ||
        (unsigned long)sizeof(struct mini_thread_tcb) > mapping_end - tp) {
        (void)mini_sys_munmap((void *)mapping, reserve);
        return 0;
    }

    tls_start = tp - mini_tls_template.block_size;
    for (index = 0UL; index < mini_tls_template.block_size; ++index) {
        ((unsigned char *)tls_start)[index] = 0U;
    }
    for (index = 0UL; index < mini_tls_template.file_size; ++index) {
        ((unsigned char *)tls_start)[index] = mini_tls_template.image[index];
    }

    tcb = (struct mini_thread_tcb *)tp;
    initialize_tcb(tcb, control);
    context->tcb = tcb;
    context->mapping = (void *)mapping;
    context->mapping_size = reserve;
    return 1;
}

int __mini_thread_context_destroy(struct mini_thread_context *context)
{
    long result;

    if (context->mapping == (void *)0) {
        return 1;
    }
    result = mini_sys_munmap(context->mapping, context->mapping_size);
    if (raw_failed(result)) {
        return 0;
    }
    context->mapping = (void *)0;
    context->mapping_size = 0UL;
    context->tcb = (struct mini_thread_tcb *)0;
    return 1;
}

void __mini_thread_runtime_init_main(char **envp)
{
    long result;

    if (!discover_static_tls(envp) ||
        !__mini_thread_context_init(&mini_main_context, &mini_main_tcb,
                                    (void *)0)) {
        mini_sys_exit(MINI_THREAD_INIT_FAILURE);
    }

    result = mini_sys_arch_prctl(MINI_ARCH_SET_FS,
                                 (unsigned long)mini_main_context.tcb);
    if (result < 0L) {
        mini_sys_exit(MINI_THREAD_INIT_FAILURE);
    }

    __mini_errno_set_provider(thread_errno_location);
}
