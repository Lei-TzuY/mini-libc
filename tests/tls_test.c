#include "../src/internal/thread_runtime.h"

#define TEST_AT_NULL 0UL
#define TEST_AT_PHDR 3UL
#define TEST_AT_PHENT 4UL
#define TEST_AT_PHNUM 5UL
#define TEST_PT_LOAD 1U
#define TEST_PT_TLS 7U

struct test_elf64_phdr {
    unsigned int p_type;
    unsigned int p_flags;
    unsigned long p_offset;
    unsigned long p_vaddr;
    unsigned long p_paddr;
    unsigned long p_filesz;
    unsigned long p_memsz;
    unsigned long p_align;
};

struct test_thread_state {
    _Alignas(MINI_COMPILER_TLS_ALIGNMENT)
        unsigned char compiler_tls[MINI_COMPILER_TLS_CAPACITY];
    struct mini_thread_tcb tcb;
};

static void build_stack(unsigned long *stack, struct test_elf64_phdr *phdr,
                        unsigned long phnum)
{
    stack[0] = 0UL;
    stack[1] = 0UL;
    stack[2] = 0UL;
    stack[3] = TEST_AT_PHDR;
    stack[4] = (unsigned long)phdr;
    stack[5] = TEST_AT_PHENT;
    stack[6] = sizeof(*phdr);
    stack[7] = TEST_AT_PHNUM;
    stack[8] = phnum;
    stack[9] = TEST_AT_NULL;
    stack[10] = 0UL;
}

static void fill(unsigned char *buffer, unsigned long size, unsigned char value)
{
    unsigned long i;

    for (i = 0UL; i < size; ++i) {
        buffer[i] = value;
    }
}

int main(void)
{
    static const unsigned char template_bytes[4] = {1U, 2U, 3U, 4U};
    struct test_elf64_phdr phdr;
    struct test_thread_state state;
    unsigned long stack[11];
    unsigned char *thread_pointer;
    unsigned char *block;
    unsigned long i;

    phdr.p_type = TEST_PT_TLS;
    phdr.p_flags = 0U;
    phdr.p_offset = 0UL;
    phdr.p_vaddr = (unsigned long)template_bytes;
    phdr.p_paddr = 0UL;
    phdr.p_filesz = 9UL;
    phdr.p_memsz = 8UL;
    phdr.p_align = 4UL;
    build_stack(stack, &phdr, 1UL);
    if (__mini_thread_tls_discover((long *)stack)) {
        return 1;
    }

    phdr.p_filesz = 0UL;
    phdr.p_memsz = MINI_COMPILER_TLS_CAPACITY + 1UL;
    phdr.p_align = 1UL;
    if (__mini_thread_tls_discover((long *)stack)) {
        return 2;
    }

    phdr.p_memsz = 8UL;
    phdr.p_align = MINI_COMPILER_TLS_ALIGNMENT * 2UL;
    if (__mini_thread_tls_discover((long *)stack)) {
        return 3;
    }

    phdr.p_filesz = sizeof(template_bytes);
    phdr.p_memsz = 8UL;
    phdr.p_align = 4UL;
    if (!__mini_thread_tls_discover((long *)stack)) {
        return 4;
    }

    fill(state.compiler_tls, MINI_COMPILER_TLS_CAPACITY, 0xa5U);
    state.tcb.self = &state.tcb;
    state.tcb.control = (void *)0;
    state.tcb.errno_value = 0;
    state.tcb.reserved = 0U;
    if (!__mini_thread_tls_prepare(&state.tcb)) {
        return 5;
    }

    thread_pointer = (unsigned char *)&state.tcb;
    if ((unsigned long)(thread_pointer - state.compiler_tls) !=
        MINI_COMPILER_TLS_CAPACITY) {
        return 6;
    }
    block = thread_pointer - 8UL;
    for (i = 0UL; i < sizeof(template_bytes); ++i) {
        if (block[i] != template_bytes[i]) {
            return 7;
        }
    }
    for (i = sizeof(template_bytes); i < 8UL; ++i) {
        if (block[i] != 0U) {
            return 8;
        }
    }
    if (state.compiler_tls[0] != 0xa5U ||
        state.compiler_tls[MINI_COMPILER_TLS_CAPACITY - 9U] != 0xa5U) {
        return 9;
    }

    phdr.p_type = TEST_PT_LOAD;
    phdr.p_filesz = 0UL;
    phdr.p_memsz = 0UL;
    phdr.p_align = 1UL;
    if (!__mini_thread_tls_discover((long *)stack)) {
        return 10;
    }
    fill(state.compiler_tls, MINI_COMPILER_TLS_CAPACITY, 0x5aU);
    if (!__mini_thread_tls_prepare(&state.tcb) ||
        state.compiler_tls[MINI_COMPILER_TLS_CAPACITY - 1U] != 0x5aU) {
        return 11;
    }

    return 0;
}
