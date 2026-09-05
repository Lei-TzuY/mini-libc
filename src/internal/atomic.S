    .bss
    .align 4
mini_stdio_lock_word:
    .long 0

    .text

    .globl __mini_atomic_exchange_int
    .hidden __mini_atomic_exchange_int
    .type __mini_atomic_exchange_int, @function
__mini_atomic_exchange_int:
    movl %esi, %eax
    xchgl %eax, (%rdi)
    ret
    .size __mini_atomic_exchange_int, .-__mini_atomic_exchange_int

    .globl __mini_atomic_fetch_add_int
    .hidden __mini_atomic_fetch_add_int
    .type __mini_atomic_fetch_add_int, @function
__mini_atomic_fetch_add_int:
    movl %esi, %eax
    lock xaddl %eax, (%rdi)
    ret
    .size __mini_atomic_fetch_add_int, .-__mini_atomic_fetch_add_int

    .globl __mini_atomic_load_ulong
    .hidden __mini_atomic_load_ulong
    .type __mini_atomic_load_ulong, @function
__mini_atomic_load_ulong:
    movq (%rdi), %rax
    ret
    .size __mini_atomic_load_ulong, .-__mini_atomic_load_ulong

    .globl __mini_atomic_exchange_ulong
    .hidden __mini_atomic_exchange_ulong
    .type __mini_atomic_exchange_ulong, @function
__mini_atomic_exchange_ulong:
    movq %rsi, %rax
    xchgq %rax, (%rdi)
    ret
    .size __mini_atomic_exchange_ulong, .-__mini_atomic_exchange_ulong

    .globl __mini_stdio_lock
    .hidden __mini_stdio_lock
    .type __mini_stdio_lock, @function
__mini_stdio_lock:
    movl %fs:20, %eax
    testl %eax, %eax
    jne .Lstdio_lock_recursive

.Lstdio_lock_try:
    movl $1, %eax
    xchgl %eax, mini_stdio_lock_word(%rip)
    testl %eax, %eax
    je .Lstdio_lock_acquired

    leaq mini_stdio_lock_word(%rip), %rdi
    xorl %esi, %esi
    movl $1, %edx
    xorl %ecx, %ecx
    xorl %r8d, %r8d
    xorl %r9d, %r9d
    subq $8, %rsp
    call mini_sys_futex
    addq $8, %rsp
    jmp .Lstdio_lock_try

.Lstdio_lock_acquired:
    movl $1, %fs:20
    ret

.Lstdio_lock_recursive:
    incl %eax
    movl %eax, %fs:20
    ret
    .size __mini_stdio_lock, .-__mini_stdio_lock

    .globl __mini_stdio_unlock
    .hidden __mini_stdio_unlock
    .type __mini_stdio_unlock, @function
__mini_stdio_unlock:
    movl %fs:20, %eax
    cmpl $1, %eax
    ja .Lstdio_unlock_recursive

    movl $0, %fs:20
    xorl %eax, %eax
    xchgl %eax, mini_stdio_lock_word(%rip)
    testl %eax, %eax
    je .Lstdio_unlock_done

    leaq mini_stdio_lock_word(%rip), %rdi
    movl $1, %esi
    movl $1, %edx
    xorl %ecx, %ecx
    xorl %r8d, %r8d
    xorl %r9d, %r9d
    subq $8, %rsp
    call mini_sys_futex
    addq $8, %rsp
.Lstdio_unlock_done:
    ret

.Lstdio_unlock_recursive:
    decl %eax
    movl %eax, %fs:20
    ret
    .size __mini_stdio_unlock, .-__mini_stdio_unlock

    .section .note.GNU-stack,"",@progbits
