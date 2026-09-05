.text

.globl scanf
.type scanf, @function
scanf:
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
    movq __mini_stdin(%rip), %rdi
    movq %rsp, %rdx
    call __mini_scan_dispatch

    addq $72, %rsp
    ret
.size scanf, .-scanf

.globl fscanf
.type fscanf, @function
fscanf:
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
    call __mini_scan_dispatch

    addq $72, %rsp
    ret
.size fscanf, .-fscanf

.section .note.GNU-stack,"",@progbits
