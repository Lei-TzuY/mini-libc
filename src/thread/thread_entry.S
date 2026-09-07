    .text

    .equ MINI_THREAD_TP_OFFSET, 4096

    .globl __mini_thread_current_tcb
    .hidden __mini_thread_current_tcb
    .type __mini_thread_current_tcb, @function
__mini_thread_current_tcb:
    movq %fs:0, %rax
    subq $MINI_THREAD_TP_OFFSET, %rax
    ret
    .size __mini_thread_current_tcb, .-__mini_thread_current_tcb

    .globl __mini_clone_thread
    .hidden __mini_clone_thread
    .type __mini_clone_thread, @function
__mini_clone_thread:
    /*
     * Normalize the caller-provided mini TCB into the native x86-64 thread
     * pointer layout before handing it to CLONE_SETTLS. Keep the five C
     * arguments across the helper call; 40 bytes also restores 16-byte call
     * alignment from the SysV entry state.
     */
    subq $40, %rsp
    movq %rdi, 0(%rsp)
    movq %rsi, 8(%rsp)
    movq %rdx, 16(%rsp)
    movq %rcx, 24(%rsp)
    movq %r8, 32(%rsp)
    movq %r8, %rdi
    call __mini_thread_prepare_tcb
    testq %rax, %rax
    je .Lmini_clone_bad_tls
    movq 0(%rsp), %rdi
    movq 8(%rsp), %rsi
    movq 16(%rsp), %rdx
    movq 24(%rsp), %rcx
    movq %rax, %r8
    addq $40, %rsp

    subq $16, %rsi
    movq %rdi, (%rsi)
    movq %rdx, %rdi
    xorl %edx, %edx
    movq %rcx, %r10
    movl $56, %eax
    syscall
    testq %rax, %rax
    jne .Lmini_clone_parent

    movq (%rsp), %rdi
    call __mini_thread_run
    ud2

.Lmini_clone_bad_tls:
    addq $40, %rsp
    movq $-22, %rax
.Lmini_clone_parent:
    ret
    .size __mini_clone_thread, .-__mini_clone_thread

    .section .note.GNU-stack,"",@progbits
