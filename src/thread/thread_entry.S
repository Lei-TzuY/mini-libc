    .text

    .globl __mini_thread_current_tcb
    .hidden __mini_thread_current_tcb
    .type __mini_thread_current_tcb, @function
__mini_thread_current_tcb:
    movq %fs:0, %rax
    ret
    .size __mini_thread_current_tcb, .-__mini_thread_current_tcb

    .globl __mini_atomic_exchange_int
    .hidden __mini_atomic_exchange_int
    .type __mini_atomic_exchange_int, @function
__mini_atomic_exchange_int:
    movl %esi, %eax
    xchgl %eax, (%rdi)
    ret
    .size __mini_atomic_exchange_int, .-__mini_atomic_exchange_int

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

    movq (%rsp), %rdi
    call __mini_thread_run
    ud2

.Lmini_clone_parent:
    ret
    .size __mini_clone_thread, .-__mini_clone_thread

    .section .note.GNU-stack,"",@progbits
