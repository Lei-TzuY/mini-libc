.text

.globl __mini_fenv_read
.type __mini_fenv_read,@function
__mini_fenv_read:
    fnstcw 0(%rdi)
    fnstsw %ax
    andw $0x003f, %ax
    movw %ax, 2(%rdi)
    stmxcsr 4(%rdi)
    ret
.size __mini_fenv_read, .-__mini_fenv_read

.globl __mini_fenv_write
.type __mini_fenv_write,@function
__mini_fenv_write:
    subq $520, %rsp
    fxsave (%rsp)

    movzwl 0(%rdi), %eax
    movw %ax, 0(%rsp)

    movzwl 2(%rdi), %eax
    andw $0x003f, %ax
    movw 2(%rsp), %dx
    andw $0xffc0, %dx
    orw %ax, %dx
    movw %dx, 2(%rsp)

    movl 4(%rdi), %eax
    movl %eax, 24(%rsp)

    fxrstor (%rsp)
    addq $520, %rsp
    ret
.size __mini_fenv_write, .-__mini_fenv_write

.section .note.GNU-stack,"",@progbits
