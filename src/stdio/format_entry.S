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

.globl vfprintf
.type vfprintf, @function
vfprintf:
    subq $72, %rsp
    movq %rdx, %r10

    movq 8(%r10), %rax
    movq %rax, 40(%rsp)
    movl $0, 48(%rsp)

    movl 0(%r10), %eax
    cmpl $48, %eax
    jae .Lvfprintf_no_gp
    movl $48, %ecx
    subl %eax, %ecx
    shrl $3, %ecx
    cmpl $5, %ecx
    jbe .Lvfprintf_count_ready
    movl $5, %ecx
.Lvfprintf_count_ready:
    movl %ecx, 52(%rsp)
    movq 16(%r10), %r8
    addq %rax, %r8
    xorl %edx, %edx
.Lvfprintf_copy_gp:
    cmpl %ecx, %edx
    jae .Lvfprintf_ready
    movq (%r8,%rdx,8), %r9
    movq %r9, (%rsp,%rdx,8)
    incl %edx
    jmp .Lvfprintf_copy_gp
.Lvfprintf_no_gp:
    movl $0, 52(%rsp)
.Lvfprintf_ready:
    movq %rsp, %rdx
    call __mini_format_dispatch

    addq $72, %rsp
    ret
.size vfprintf, .-vfprintf

.globl vprintf
.type vprintf, @function
vprintf:
    movq %rsi, %rdx
    movq %rdi, %rsi
    movq __mini_stdout(%rip), %rdi
    jmp vfprintf
.size vprintf, .-vprintf

.globl vsnprintf
.type vsnprintf, @function
vsnprintf:
    subq $72, %rsp
    movq %rdx, %r11
    movq %rcx, %r10

    movq 8(%r10), %rax
    movq %rax, 40(%rsp)
    movl $0, 48(%rsp)

    movl 0(%r10), %eax
    cmpl $48, %eax
    jae .Lvsnprintf_no_gp
    movl $48, %ecx
    subl %eax, %ecx
    shrl $3, %ecx
    cmpl $5, %ecx
    jbe .Lvsnprintf_count_ready
    movl $5, %ecx
.Lvsnprintf_count_ready:
    movl %ecx, 52(%rsp)
    movq 16(%r10), %r8
    addq %rax, %r8
    xorl %edx, %edx
.Lvsnprintf_copy_gp:
    cmpl %ecx, %edx
    jae .Lvsnprintf_ready
    movq (%r8,%rdx,8), %r9
    movq %r9, (%rsp,%rdx,8)
    incl %edx
    jmp .Lvsnprintf_copy_gp
.Lvsnprintf_no_gp:
    movl $0, 52(%rsp)
.Lvsnprintf_ready:
    movq %r11, %rdx
    movq %rsp, %rcx
    call __mini_snprintf_dispatch

    addq $72, %rsp
    ret
.size vsnprintf, .-vsnprintf

.section .note.GNU-stack,"",@progbits
