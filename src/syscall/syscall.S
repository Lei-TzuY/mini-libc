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

    .globl mini_sys_openat
    .type mini_sys_openat, @function
mini_sys_openat:
    movq %rcx, %r10
    movl $257, %eax
    syscall
    ret
    .size mini_sys_openat, .-mini_sys_openat

    .globl mini_sys_exit
    .type mini_sys_exit, @function
mini_sys_exit:
    movl $60, %eax
    syscall
    ud2
    .size mini_sys_exit, .-mini_sys_exit

    .section .note.GNU-stack,"",@progbits
