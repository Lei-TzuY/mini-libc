#include <mini/syscall.h>
#include <threads.h>

#include "../internal/thread_runtime.h"

#define MINI_ARCH_SET_FS 0x1002
#define MINI_THREAD_INIT_FAILURE 127

#define MINI_AT_NULL 0UL
#define MINI_AT_PHDR 3UL
#define MINI_AT_PHENT 4UL
#define MINI_AT_PHNUM 5UL
#define MINI_PT_TLS 7U
#define MINI_PT_PHDR 6U

struct mini_elf_program_header {
    unsigned int type;
    unsigned int flags;
    unsigned long offset;
    unsigned long virtual_address;
    unsigned long physical_address;
    unsigned long file_size;
    unsigned long memory_size;
    unsigned long alignment;
};

static struct mini_thread_tcb mini_main_tcb;
static const unsigned char *mini_tls_template;
static unsigned long mini_tls_file_size;
static unsigned long mini_tls_block_size;
static unsigned long mini_tls_alignment = 1UL;

static int *thread_errno_location(void)
{
    struct mini_thread_tcb *tcb = __mini_thread_current_tcb();

    return &tcb->errno_value;
}

static int raw_failed(long value)
{
    return value < 0L && value >= -4095L;
}

static int align_up(unsigned long value, unsigned long alignment,
                    unsigned long *result)
{
    unsigned long remainder;
    unsigned long addition;

    if (alignment == 0UL) {
        alignment = 1UL;
    }
    remainder = value % alignment;
    if (remainder == 0UL) {
        *result = value;
        return 1;
    }
    addition = alignment - remainder;
    if (value > ~0UL - addition) {
        return 0;
    }
    *result = value + addition;
    return 1;
}

static const struct mini_elf_program_header *program_header_at(
    unsigned long table, unsigned long entry_size, unsigned long index)
{
    return (const struct mini_elf_program_header *)(
        table + entry_size * index);
}

static void configure_native_tls(long *initial_stack)
{
    unsigned long phdr_address = 0UL;
    unsigned long phent = 0UL;
    unsigned long phnum = 0UL;
    unsigned long load_bias = 0UL;
    unsigned long *cursor;
    unsigned long index;
    long argc;

    if (initial_stack == (long *)0) {
        return;
    }
    argc = initial_stack[0];
    if (argc < 0L) {
        mini_sys_exit(MINI_THREAD_INIT_FAILURE);
    }

    cursor = (unsigned long *)(initial_stack + 1L + argc + 1L);
    while (*cursor != 0UL) {
        ++cursor;
    }
    ++cursor;

    while (cursor[0] != MINI_AT_NULL) {
        if (cursor[0] == MINI_AT_PHDR) {
            phdr_address = cursor[1];
        } else if (cursor[0] == MINI_AT_PHENT) {
            phent = cursor[1];
        } else if (cursor[0] == MINI_AT_PHNUM) {
            phnum = cursor[1];
        }
        cursor += 2;
    }

    if (phdr_address == 0UL || phnum == 0UL) {
        return;
    }
    if (phent < sizeof(struct mini_elf_program_header)) {
        mini_sys_exit(MINI_THREAD_INIT_FAILURE);
    }

    for (index = 0UL; index < phnum; ++index) {
        const struct mini_elf_program_header *header =
            program_header_at(phdr_address, phent, index);

        if (header->type == MINI_PT_PHDR) {
            if (phdr_address < header->virtual_address) {
                mini_sys_exit(MINI_THREAD_INIT_FAILURE);
            }
            load_bias = phdr_address - header->virtual_address;
            break;
        }
    }

    for (index = 0UL; index < phnum; ++index) {
        const struct mini_elf_program_header *header =
            program_header_at(phdr_address, phent, index);
        unsigned long alignment;
        unsigned long block_size;
        unsigned long template_address;

        if (header->type != MINI_PT_TLS) {
            continue;
        }
        if (header->file_size > header->memory_size) {
            mini_sys_exit(MINI_THREAD_INIT_FAILURE);
        }

        alignment = header->alignment == 0UL ? 1UL : header->alignment;
        if ((alignment & (alignment - 1UL)) != 0UL ||
            alignment > MINI_NATIVE_TLS_MAX_ALIGNMENT ||
            !align_up(header->memory_size, alignment, &block_size) ||
            block_size > MINI_NATIVE_TLS_STORAGE_SIZE) {
            mini_sys_exit(MINI_THREAD_INIT_FAILURE);
        }
        if (header->virtual_address > ~0UL - load_bias) {
            mini_sys_exit(MINI_THREAD_INIT_FAILURE);
        }
        template_address = header->virtual_address + load_bias;

        mini_tls_template = (const unsigned char *)template_address;
        mini_tls_file_size = header->file_size;
        mini_tls_block_size = block_size;
        mini_tls_alignment = alignment;
        return;
    }
}

void *__mini_thread_prepare_tcb(struct mini_thread_tcb *tcb)
{
    unsigned char *thread_pointer;
    unsigned long index;

    if (tcb == (struct mini_thread_tcb *)0) {
        return (void *)0;
    }
    thread_pointer = (unsigned char *)&tcb->self;
    if (((unsigned long)thread_pointer & (mini_tls_alignment - 1UL)) != 0UL) {
        return (void *)0;
    }

    if (mini_tls_block_size != 0UL) {
        unsigned char *tls_base = thread_pointer - mini_tls_block_size;

        for (index = 0UL; index < mini_tls_block_size; ++index) {
            tls_base[index] = 0U;
        }
        for (index = 0UL; index < mini_tls_file_size; ++index) {
            tls_base[index] = mini_tls_template[index];
        }
    }

    tcb->self = (struct mini_thread_tcb *)thread_pointer;
    return thread_pointer;
}

void __mini_thread_runtime_init_main(long *initial_stack)
{
    void *thread_pointer;
    long result;

    configure_native_tls(initial_stack);

    mini_main_tcb.control = (void *)0;
    mini_main_tcb.errno_value = 0;
    mini_main_tcb.reserved = 0U;
    thread_pointer = __mini_thread_prepare_tcb(&mini_main_tcb);
    if (thread_pointer == (void *)0) {
        mini_sys_exit(MINI_THREAD_INIT_FAILURE);
    }

    result = mini_sys_arch_prctl(MINI_ARCH_SET_FS,
                                 (unsigned long)thread_pointer);
    if (raw_failed(result)) {
        mini_sys_exit(MINI_THREAD_INIT_FAILURE);
    }

    __mini_errno_set_provider(thread_errno_location);
}

void thrd_yield(void)
{
    (void)mini_sys_sched_yield();
}
