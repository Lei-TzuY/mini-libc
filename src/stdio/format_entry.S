.text

.globl printf
.type printf, @function
printf:
    movzbl %al, %r11d
    subq $136, %rsp

    movq %rsi, 0(%rsp)
    movq %rdx, 8(%rsp)
    movq %rcx, 16(%rsp)
    movq %r8, 24(%rsp)
    movq %r9, 32(%rsp)
    movsd %xmm0, 40(%rsp)
    movsd %xmm1, 48(%rsp)
    movsd %xmm2, 56(%rsp)
    movsd %xmm3, 64(%rsp)
    movsd %xmm4, 72(%rsp)
    movsd %xmm5, 80(%rsp)
    movsd %xmm6, 88(%rsp)
    movsd %xmm7, 96(%rsp)
    leaq 144(%rsp), %rax
    movq %rax, 104(%rsp)
    movl $0, 112(%rsp)
    movl $5, 116(%rsp)
    movl $0, 120(%rsp)
    cmpl $8, %r11d
    jbe .Lprintf_fp_count_ready
    movl $8, %r11d
.Lprintf_fp_count_ready:
    movl %r11d, 124(%rsp)

    movq %rdi, %rsi
    movq __mini_stdout(%rip), %rdi
    movq %rsp, %rdx
    call __mini_format_dispatch

    addq $136, %rsp
    ret
.size printf, .-printf

.globl fprintf
.type fprintf, @function
fprintf:
    movzbl %al, %r11d
    subq $136, %rsp

    movq %rdx, 0(%rsp)
    movq %rcx, 8(%rsp)
    movq %r8, 16(%rsp)
    movq %r9, 24(%rsp)
    movq $0, 32(%rsp)
    movsd %xmm0, 40(%rsp)
    movsd %xmm1, 48(%rsp)
    movsd %xmm2, 56(%rsp)
    movsd %xmm3, 64(%rsp)
    movsd %xmm4, 72(%rsp)
    movsd %xmm5, 80(%rsp)
    movsd %xmm6, 88(%rsp)
    movsd %xmm7, 96(%rsp)
    leaq 144(%rsp), %rax
    movq %rax, 104(%rsp)
    movl $0, 112(%rsp)
    movl $4, 116(%rsp)
    movl $0, 120(%rsp)
    cmpl $8, %r11d
    jbe .Lfprintf_fp_count_ready
    movl $8, %r11d
.Lfprintf_fp_count_ready:
    movl %r11d, 124(%rsp)

    movq %rsp, %rdx
    call __mini_format_dispatch

    addq $136, %rsp
    ret
.size fprintf, .-fprintf

.globl snprintf
.type snprintf, @function
snprintf:
    movzbl %al, %r11d
    subq $136, %rsp

    movq %rcx, 0(%rsp)
    movq %r8, 8(%rsp)
    movq %r9, 16(%rsp)
    movq $0, 24(%rsp)
    movq $0, 32(%rsp)
    movsd %xmm0, 40(%rsp)
    movsd %xmm1, 48(%rsp)
    movsd %xmm2, 56(%rsp)
    movsd %xmm3, 64(%rsp)
    movsd %xmm4, 72(%rsp)
    movsd %xmm5, 80(%rsp)
    movsd %xmm6, 88(%rsp)
    movsd %xmm7, 96(%rsp)
    leaq 144(%rsp), %rax
    movq %rax, 104(%rsp)
    movl $0, 112(%rsp)
    movl $3, 116(%rsp)
    movl $0, 120(%rsp)
    cmpl $8, %r11d
    jbe .Lsnprintf_fp_count_ready
    movl $8, %r11d
.Lsnprintf_fp_count_ready:
    movl %r11d, 124(%rsp)

    movq %rsp, %rcx
    call __mini_snprintf_dispatch

    addq $136, %rsp
    ret
.size snprintf, .-snprintf

.globl vfprintf
.type vfprintf, @function
vfprintf:
    subq $136, %rsp
    movq %rdx, %r10

    movq 8(%r10), %rax
    movq %rax, 104(%rsp)
    movl $0, 112(%rsp)
    movl $0, 120(%rsp)

    movl 0(%r10), %eax
    cmpl $48, %eax
    jae .Lvfprintf_no_gp
    movl $48, %ecx
    subl %eax, %ecx
    shrl $3, %ecx
    cmpl $5, %ecx
    jbe .Lvfprintf_gp_count_ready
    movl $5, %ecx
.Lvfprintf_gp_count_ready:
    movl %ecx, 116(%rsp)
    movq 16(%r10), %r8
    addq %rax, %r8
    xorl %edx, %edx
.Lvfprintf_copy_gp:
    cmpl %ecx, %edx
    jae .Lvfprintf_gp_ready
    movq (%r8,%rdx,8), %r9
    movq %r9, (%rsp,%rdx,8)
    incl %edx
    jmp .Lvfprintf_copy_gp
.Lvfprintf_no_gp:
    movl $0, 116(%rsp)
.Lvfprintf_gp_ready:

    movl 4(%r10), %eax
    cmpl $176, %eax
    jae .Lvfprintf_no_fp
    movl $176, %ecx
    subl %eax, %ecx
    shrl $4, %ecx
    cmpl $8, %ecx
    jbe .Lvfprintf_fp_count_ready
    movl $8, %ecx
.Lvfprintf_fp_count_ready:
    movl %ecx, 124(%rsp)
    movq 16(%r10), %r8
    addq %rax, %r8
    xorl %edx, %edx
.Lvfprintf_copy_fp:
    cmpl %ecx, %edx
    jae .Lvfprintf_ready
    movq (%r8), %r9
    movq %r9, 40(%rsp,%rdx,8)
    addq $16, %r8
    incl %edx
    jmp .Lvfprintf_copy_fp
.Lvfprintf_no_fp:
    movl $0, 124(%rsp)
.Lvfprintf_ready:
    movq %rsp, %rdx
    call __mini_format_dispatch

    addq $136, %rsp
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
    subq $136, %rsp
    movq %rdx, %r11
    movq %rcx, %r10

    movq 8(%r10), %rax
    movq %rax, 104(%rsp)
    movl $0, 112(%rsp)
    movl $0, 120(%rsp)

    movl 0(%r10), %eax
    cmpl $48, %eax
    jae .Lvsnprintf_no_gp
    movl $48, %ecx
    subl %eax, %ecx
    shrl $3, %ecx
    cmpl $5, %ecx
    jbe .Lvsnprintf_gp_count_ready
    movl $5, %ecx
.Lvsnprintf_gp_count_ready:
    movl %ecx, 116(%rsp)
    movq 16(%r10), %r8
    addq %rax, %r8
    xorl %edx, %edx
.Lvsnprintf_copy_gp:
    cmpl %ecx, %edx
    jae .Lvsnprintf_gp_ready
    movq (%r8,%rdx,8), %r9
    movq %r9, (%rsp,%rdx,8)
    incl %edx
    jmp .Lvsnprintf_copy_gp
.Lvsnprintf_no_gp:
    movl $0, 116(%rsp)
.Lvsnprintf_gp_ready:

    movl 4(%r10), %eax
    cmpl $176, %eax
    jae .Lvsnprintf_no_fp
    movl $176, %ecx
    subl %eax, %ecx
    shrl $4, %ecx
    cmpl $8, %ecx
    jbe .Lvsnprintf_fp_count_ready
    movl $8, %ecx
.Lvsnprintf_fp_count_ready:
    movl %ecx, 124(%rsp)
    movq 16(%r10), %r8
    addq %rax, %r8
    xorl %edx, %edx
.Lvsnprintf_copy_fp:
    cmpl %ecx, %edx
    jae .Lvsnprintf_ready
    movq (%r8), %r9
    movq %r9, 40(%rsp,%rdx,8)
    addq $16, %r8
    incl %edx
    jmp .Lvsnprintf_copy_fp
.Lvsnprintf_no_fp:
    movl $0, 124(%rsp)
.Lvsnprintf_ready:
    movq %r11, %rdx
    movq %rsp, %rcx
    call __mini_snprintf_dispatch

    addq $136, %rsp
    ret
.size vsnprintf, .-vsnprintf

.section .note.GNU-stack,"",@progbits