.text

.globl printf
.type printf, @function
printf:
    subq $72, %rsp

    movq %rsi, 0(%rsp)
    movq %rdx, 8(%rsp)
    movq %rcx, 16(%rsp)
    movq %r8, 24(%rsp)
    movq %r9, 32(%rsp)
    leaq 80(%rsp), %rax
    movq %rax, 40(%rsp)
    movl $0, 48(%rsp)
    movl $5, 52(%rsp)

    movq %rdi, %rsi
    movq __mini_stdout(%rip), %rdi
    movq %rsp, %rdx
    call __mini_format_dispatch

    addq $72, %rsp
    ret
.size printf, .-printf

.globl fprintf
.type fprintf, @function
fprintf:
    subq $72, %rsp

    movq %rdx, 0(%rsp)
    movq %rcx, 8(%rsp)
    movq %r8, 16(%rsp)
    movq %r9, 24(%rsp)
    movq $0, 32(%rsp)
    leaq 80(%rsp), %rax
    movq %rax, 40(%rsp)
    movl $0, 48(%rsp)
    movl $4, 52(%rsp)

    movq %rsp, %rdx
    call __mini_format_dispatch

    addq $72, %rsp
    ret
.size fprintf, .-fprintf

.globl snprintf
.type snprintf, @function
snprintf:
    subq $72, %rsp

    movq %rcx, 0(%rsp)
    movq %r8, 8(%rsp)
    movq %r9, 16(%rsp)
    movq $0, 24(%rsp)
    movq $0, 32(%rsp)
    leaq 80(%rsp), %rax
    movq %rax, 40(%rsp)
    movl $0, 48(%rsp)
    movl $3, 52(%rsp)

    movq %rsp, %rcx
    call __mini_snprintf_dispatch

    addq $72, %rsp
    ret
.size snprintf, .-snprintf

.section .note.GNU-stack,"",@progbits
