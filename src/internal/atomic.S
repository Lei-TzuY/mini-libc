    .text

    .globl __mini_atomic_exchange_int
    .hidden __mini_atomic_exchange_int
    .type __mini_atomic_exchange_int, @function
__mini_atomic_exchange_int:
    movl %esi, %eax
    xchgl %eax, (%rdi)
    ret
    .size __mini_atomic_exchange_int, .-__mini_atomic_exchange_int

    .section .note.GNU-stack,"",@progbits
