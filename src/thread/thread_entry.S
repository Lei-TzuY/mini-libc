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
    subq $16, %rsi
    movq %rdi, (%rsi)
    movq %rdx, %rdi
    xorl %edx, %edx
    movq %rcx, %r10
    movl $56, %eax
    syscall
    testq %rax, %rax
    jne .Lmini_clone_parent

    movq %fs:0, %rdi
    call __mini_thread_tls_prepare
    testl %eax, %eax
    jne .Lmini_tls_ready
    movl $127, %edi
    call mini_sys_exit
    ud2

.Lmini_tls_ready:
    movq (%rsp), %rdi
    call __mini_thread_run
    ud2

.Lmini_clone_parent:
    ret
    .size __mini_clone_thread, .-__mini_clone_thread

    .section .note.GNU-stack,"",@progbits
