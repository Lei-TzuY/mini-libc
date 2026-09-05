.text

.globl mini_test_setjmp_registers
.type mini_test_setjmp_registers, @function
mini_test_setjmp_registers:
    pushq %rbx
    pushq %rbp
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    subq $8, %rsp
    movq %rdi, (%rsp)

    movq $0x11111111, %rbx
    movq $0x22222222, %rbp
    movq $0x33333333, %r12
    movq $0x44444444, %r13
    movq $0x55555555, %r14
    movq $0x66666666, %r15

    call setjmp
    testl %eax, %eax
    jne .Lresumed

    xorq %rbx, %rbx
    xorq %rbp, %rbp
    xorq %r12, %r12
    xorq %r13, %r13
    xorq %r14, %r14
    xorq %r15, %r15
    movq (%rsp), %rdi
    movl $5, %esi
    call longjmp
    ud2

.Lresumed:
    cmpl $5, %eax
    jne .Lfailed
    cmpq $0x11111111, %rbx
    jne .Lfailed
    cmpq $0x22222222, %rbp
    jne .Lfailed
    cmpq $0x33333333, %r12
    jne .Lfailed
    cmpq $0x44444444, %r13
    jne .Lfailed
    cmpq $0x55555555, %r14
    jne .Lfailed
    cmpq $0x66666666, %r15
    jne .Lfailed
    xorl %eax, %eax
    jmp .Lcleanup

.Lfailed:
    movl $1, %eax
.Lcleanup:
    addq $8, %rsp
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbp
    popq %rbx
    ret
.size mini_test_setjmp_registers, .-mini_test_setjmp_registers

.section .note.GNU-stack,"",@progbits
