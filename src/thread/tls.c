#include "../internal/thread_runtime.h"

#define MINI_AT_NULL 0UL
#define MINI_AT_PHDR 3UL
#define MINI_AT_PHENT 4UL
#define MINI_AT_PHNUM 5UL
#define MINI_PT_TLS 7U
#define MINI_AUXV_LIMIT 64U
#define MINI_PHDR_LIMIT 128UL

struct mini_elf64_phdr {
    unsigned int p_type;
    unsigned int p_flags;
    unsigned long p_offset;
    unsigned long p_vaddr;
    unsigned long p_paddr;
    unsigned long p_filesz;
    unsigned long p_memsz;
    unsigned long p_align;
};

struct mini_tls_template {
    const unsigned char *image;
    unsigned long file_size;
    unsigned long memory_size;
    unsigned long block_size;
    unsigned long alignment;
};

static struct mini_tls_template mini_tls_template;

static int power_of_two(unsigned long value)
{
    return value != 0UL && (value & (value - 1UL)) == 0UL;
}

static int align_up(unsigned long value, unsigned long alignment,
                    unsigned long *result)
{
    unsigned long mask = alignment - 1UL;

    if (value > ~0UL - mask) {
        return 0;
    }
    *result = (value + mask) & ~mask;
    return 1;
}

int __mini_thread_tls_discover(long *initial_stack)
{
    unsigned long *cursor;
    unsigned long phdr_address = 0UL;
    unsigned long phent = 0UL;
    unsigned long phnum = 0UL;
    unsigned long raw_argc;
    unsigned int aux_index;
    struct mini_tls_template candidate = {(const unsigned char *)0, 0UL, 0UL,
                                          0UL, 1UL};
    int found_tls = 0;

    if (initial_stack == (long *)0 || initial_stack[0] < 0L) {
        return 0;
    }
    raw_argc = (unsigned long)initial_stack[0];
    if (raw_argc > 65536UL) {
        return 0;
    }

    cursor = (unsigned long *)(initial_stack + 1 + raw_argc);
    if (*cursor != 0UL) {
        return 0;
    }
    ++cursor;
    while (*cursor != 0UL) {
        ++cursor;
    }
    ++cursor;

    for (aux_index = 0U; aux_index < MINI_AUXV_LIMIT; ++aux_index) {
        unsigned long type = cursor[0];
        unsigned long value = cursor[1];

        cursor += 2;
        if (type == MINI_AT_NULL) {
            break;
        }
        if (type == MINI_AT_PHDR) {
            phdr_address = value;
        } else if (type == MINI_AT_PHENT) {
            phent = value;
        } else if (type == MINI_AT_PHNUM) {
            phnum = value;
        }
    }
    if (aux_index == MINI_AUXV_LIMIT || phdr_address == 0UL ||
        phent != sizeof(struct mini_elf64_phdr) || phnum > MINI_PHDR_LIMIT) {
        return 0;
    }

    {
        unsigned long index;
        const unsigned char *phdr_bytes =
            (const unsigned char *)phdr_address;

        for (index = 0UL; index < phnum; ++index) {
            const struct mini_elf64_phdr *phdr =
                (const struct mini_elf64_phdr *)(phdr_bytes + index * phent);
            unsigned long alignment;
            unsigned long block_size;

            if (phdr->p_type != MINI_PT_TLS) {
                continue;
            }
            if (found_tls || phdr->p_filesz > phdr->p_memsz) {
                return 0;
            }
            alignment = phdr->p_align == 0UL ? 1UL : phdr->p_align;
            if (!power_of_two(alignment) ||
                alignment > MINI_COMPILER_TLS_ALIGNMENT ||
                !align_up(phdr->p_memsz, alignment, &block_size) ||
                block_size > MINI_COMPILER_TLS_CAPACITY ||
                (phdr->p_filesz != 0UL && phdr->p_vaddr == 0UL) ||
                phdr->p_vaddr > ~0UL - phdr->p_filesz) {
                return 0;
            }

            candidate.image = (const unsigned char *)phdr->p_vaddr;
            candidate.file_size = phdr->p_filesz;
            candidate.memory_size = phdr->p_memsz;
            candidate.block_size = block_size;
            candidate.alignment = alignment;
            found_tls = 1;
        }
    }

    mini_tls_template = candidate;
    return 1;
}

int __mini_thread_tls_prepare(struct mini_thread_tcb *tcb)
{
    unsigned char *thread_pointer;
    unsigned char *block;
    unsigned long index;

    if (tcb == (struct mini_thread_tcb *)0) {
        return 0;
    }
    if (mini_tls_template.block_size == 0UL) {
        return 1;
    }

    thread_pointer = (unsigned char *)tcb;
    if (((unsigned long)thread_pointer &
         (mini_tls_template.alignment - 1UL)) != 0UL) {
        return 0;
    }
    block = thread_pointer - mini_tls_template.block_size;
    for (index = 0UL; index < mini_tls_template.block_size; ++index) {
        block[index] = 0U;
    }
    for (index = 0UL; index < mini_tls_template.file_size; ++index) {
        block[index] = mini_tls_template.image[index];
    }
    return 1;
}
