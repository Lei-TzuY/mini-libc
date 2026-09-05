.text

.globl setjmp
.type setjmp, @function
setjmp:
    movq %rbx, 0(%rdi)
    movq %rbp, 8(%rdi)
    movq %r12, 16(%rdi)
    movq %r13, 24(%rdi)
    movq %r14, 32(%rdi)
    movq %r15, 40(%rdi)
    leaq 8(%rsp), %rax
    movq %rax, 48(%rdi)
    movq (%rsp), %rax
    movq %rax, 56(%rdi)
    xorl %eax, %eax
    ret
.size setjmp, .-setjmp

.globl longjmp
.type longjmp, @function
longjmp:
    movl %esi, %eax
    testl %eax, %eax
    jne .Llongjmp_value_ready
    movl $1, %eax
.Llongjmp_value_ready:
    movq 56(%rdi), %rdx
    movq 0(%rdi), %rbx
    movq 8(%rdi), %rbp
    movq 16(%rdi), %r12
    movq 24(%rdi), %r13
    movq 32(%rdi), %r14
    movq 40(%rdi), %r15
    movq 48(%rdi), %rsp
    jmp *%rdx
.size longjmp, .-longjmp

.section .note.GNU-stack,"",@progbits
