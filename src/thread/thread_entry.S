    .text

    .globl __mini_thread_current_tcb
    .hidden __mini_thread_current_tcb
    .type __mini_thread_current_tcb, @function
__mini_thread_current_tcb:
    movq %fs:0, %rax
    ret
    .size __mini_thread_current_tcb, .-__mini_thread_current_tcb

    .globl __mini_clone_thread
    .hidden __mini_clone_thread
    .type __mini_clone_thread, @function
__mini_clone_thread:
    subq $40, %rsp
    movq %rdi, 0(%rsp)
    movq %rdx, 8(%rsp)
    movq %rcx, 16(%rsp)
    movq %r8, 24(%rsp)

    leaq 32(%rsp), %rcx
    movq %rsi, %rdi
    movq %r8, %rsi
    movq 0(%rsp), %rdx
    call __mini_thread_prepare_clone_tls
    testq %rax, %rax
    je .Lmini_clone_prepare_failed

    movq %rax, %rsi
    movq 32(%rsp), %r8
    movq 0(%rsp), %r9
    movq 8(%rsp), %rdi
    movq 16(%rsp), %r10
    addq $40, %rsp

    subq $16, %rsi
    movq %r9, (%rsi)
    xorl %edx, %edx
    movl $56, %eax
    syscall
    testq %rax, %rax
    jne .Lmini_clone_parent

    movq (%rsp), %rdi
    call __mini_thread_run
    ud2

.Lmini_clone_prepare_failed:
    addq $40, %rsp
    movq $-12, %rax
.Lmini_clone_parent:
    ret
    .size __mini_clone_thread, .-__mini_clone_thread

    .section .note.GNU-stack,"",@progbits
