    .text

    .globl __mini_rt_sigreturn_restorer
    .hidden __mini_rt_sigreturn_restorer
    .type __mini_rt_sigreturn_restorer, @function
__mini_rt_sigreturn_restorer:
    movl $15, %eax
    syscall
    ud2
    .size __mini_rt_sigreturn_restorer, .-__mini_rt_sigreturn_restorer

    .section .note.GNU-stack,"",@progbits
