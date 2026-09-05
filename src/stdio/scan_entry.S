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

.globl sscanf
.type sscanf, @function
sscanf:
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
    call __mini_sscan_dispatch

    addq $72, %rsp
    ret
.size sscanf, .-sscanf

.globl vfscanf
.type vfscanf, @function
vfscanf:
    subq $72, %rsp
    movq %rdx, %r10

    movq 8(%r10), %rax
    movq %rax, 40(%rsp)
    movl $0, 48(%rsp)

    movl 0(%r10), %eax
    cmpl $48, %eax
    jae .Lvfscanf_no_gp
    movl $48, %ecx
    subl %eax, %ecx
    shrl $3, %ecx
    cmpl $5, %ecx
    jbe .Lvfscanf_count_ready
    movl $5, %ecx
.Lvfscanf_count_ready:
    movl %ecx, 52(%rsp)
    movq 16(%r10), %r8
    addq %rax, %r8
    xorl %edx, %edx
.Lvfscanf_copy_gp:
    cmpl %ecx, %edx
    jae .Lvfscanf_ready
    movq (%r8,%rdx,8), %r9
    movq %r9, (%rsp,%rdx,8)
    incl %edx
    jmp .Lvfscanf_copy_gp
.Lvfscanf_no_gp:
    movl $0, 52(%rsp)
.Lvfscanf_ready:
    movq %rsp, %rdx
    call __mini_scan_dispatch

    addq $72, %rsp
    ret
.size vfscanf, .-vfscanf

.globl vscanf
.type vscanf, @function
vscanf:
    movq %rsi, %rdx
    movq %rdi, %rsi
    movq __mini_stdin(%rip), %rdi
    jmp vfscanf
.size vscanf, .-vscanf

.globl vsscanf
.type vsscanf, @function
vsscanf:
    subq $72, %rsp
    movq %rdx, %r10

    movq 8(%r10), %rax
    movq %rax, 40(%rsp)
    movl $0, 48(%rsp)

    movl 0(%r10), %eax
    cmpl $48, %eax
    jae .Lvsscanf_no_gp
    movl $48, %ecx
    subl %eax, %ecx
    shrl $3, %ecx
    cmpl $5, %ecx
    jbe .Lvsscanf_count_ready
    movl $5, %ecx
.Lvsscanf_count_ready:
    movl %ecx, 52(%rsp)
    movq 16(%r10), %r8
    addq %rax, %r8
    xorl %edx, %edx
.Lvsscanf_copy_gp:
    cmpl %ecx, %edx
    jae .Lvsscanf_ready
    movq (%r8,%rdx,8), %r9
    movq %r9, (%rsp,%rdx,8)
    incl %edx
    jmp .Lvsscanf_copy_gp
.Lvsscanf_no_gp:
    movl $0, 52(%rsp)
.Lvsscanf_ready:
    movq %rsp, %rdx
    call __mini_sscan_dispatch

    addq $72, %rsp
    ret
.size vsscanf, .-vsscanf

.section .note.GNU-stack,"",@progbits
