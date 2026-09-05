    .text

    .globl mini_sys_read
    .type mini_sys_read, @function
mini_sys_read:
    movl $0, %eax
    syscall
    ret
    .size mini_sys_read, .-mini_sys_read

    .globl mini_sys_write
    .type mini_sys_write, @function
mini_sys_write:
    movl $1, %eax
    syscall
    ret
    .size mini_sys_write, .-mini_sys_write

    .globl mini_sys_close
    .type mini_sys_close, @function
mini_sys_close:
    movl $3, %eax
    syscall
    ret
    .size mini_sys_close, .-mini_sys_close

    .globl mini_sys_lseek
    .type mini_sys_lseek, @function
mini_sys_lseek:
    movl $8, %eax
    syscall
    ret
    .size mini_sys_lseek, .-mini_sys_lseek

    .globl mini_sys_mmap
    .type mini_sys_mmap, @function
mini_sys_mmap:
    movq %rcx, %r10
    movl $9, %eax
    syscall
    ret
    .size mini_sys_mmap, .-mini_sys_mmap

    .globl mini_sys_munmap
    .type mini_sys_munmap, @function
mini_sys_munmap:
    movl $11, %eax
    syscall
    ret
    .size mini_sys_munmap, .-mini_sys_munmap

    .globl mini_sys_brk
    .type mini_sys_brk, @function
mini_sys_brk:
    movl $12, %eax
    syscall
    ret
    .size mini_sys_brk, .-mini_sys_brk

    .globl mini_sys_rt_sigaction
    .type mini_sys_rt_sigaction, @function
mini_sys_rt_sigaction:
    movq %rcx, %r10
    movl $13, %eax
    syscall
    ret
    .size mini_sys_rt_sigaction, .-mini_sys_rt_sigaction

    .globl __mini_rt_sigreturn_restorer
    .hidden __mini_rt_sigreturn_restorer
    .type __mini_rt_sigreturn_restorer, @function
__mini_rt_sigreturn_restorer:
    movl $15, %eax
    syscall
    ud2
    .size __mini_rt_sigreturn_restorer, .-__mini_rt_sigreturn_restorer

    .globl mini_sys_getpid
    .type mini_sys_getpid, @function
mini_sys_getpid:
    movl $39, %eax
    syscall
    ret
    .size mini_sys_getpid, .-mini_sys_getpid

    .globl mini_sys_gettid
    .type mini_sys_gettid, @function
mini_sys_gettid:
    movl $186, %eax
    syscall
    ret
    .size mini_sys_gettid, .-mini_sys_gettid

    .globl mini_sys_clock_gettime
    .type mini_sys_clock_gettime, @function
mini_sys_clock_gettime:
    movl $228, %eax
    syscall
    ret
    .size mini_sys_clock_gettime, .-mini_sys_clock_gettime

    .globl mini_sys_tgkill
    .type mini_sys_tgkill, @function
mini_sys_tgkill:
    movl $234, %eax
    syscall
    ret
    .size mini_sys_tgkill, .-mini_sys_tgkill

    .globl mini_sys_openat
    .type mini_sys_openat, @function
mini_sys_openat:
    movq %rcx, %r10
    movl $257, %eax
    syscall
    ret
    .size mini_sys_openat, .-mini_sys_openat

    .globl mini_sys_unlinkat
    .type mini_sys_unlinkat, @function
mini_sys_unlinkat:
    movl $263, %eax
    syscall
    ret
    .size mini_sys_unlinkat, .-mini_sys_unlinkat

    .globl mini_sys_renameat
    .type mini_sys_renameat, @function
mini_sys_renameat:
    movq %rcx, %r10
    movl $264, %eax
    syscall
    ret
    .size mini_sys_renameat, .-mini_sys_renameat

    .globl mini_sys_exit
    .type mini_sys_exit, @function
mini_sys_exit:
    movl $60, %eax
    syscall
    ud2
    .size mini_sys_exit, .-mini_sys_exit

    .section .note.GNU-stack,"",@progbits
