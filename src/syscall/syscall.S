    .text

    .globl mini_sys_poll
    .type mini_sys_poll, @function
mini_sys_poll:
    movl $7, %eax
    syscall
    ret
    .size mini_sys_poll, .-mini_sys_poll

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

    .globl mini_sys_dup
    .type mini_sys_dup, @function
mini_sys_dup:
    movl $32, %eax
    syscall
    ret
    .size mini_sys_dup, .-mini_sys_dup

    .globl mini_sys_dup2
    .type mini_sys_dup2, @function
mini_sys_dup2:
    movl $33, %eax
    syscall
    ret
    .size mini_sys_dup2, .-mini_sys_dup2

    .globl mini_sys_fstat
    .type mini_sys_fstat, @function
mini_sys_fstat:
    movl $5, %eax
    syscall
    ret
    .size mini_sys_fstat, .-mini_sys_fstat

    .globl mini_sys_fcntl
    .type mini_sys_fcntl, @function
mini_sys_fcntl:
    movl $72, %eax
    syscall
    ret
    .size mini_sys_fcntl, .-mini_sys_fcntl

    .globl mini_sys_lseek
    .type mini_sys_lseek, @function
mini_sys_lseek:
    movl $8, %eax
    syscall
    ret
    .size mini_sys_lseek, .-mini_sys_lseek

    .globl mini_sys_fork
    .type mini_sys_fork, @function
mini_sys_fork:
    movl $57, %eax
    syscall
    ret
    .size mini_sys_fork, .-mini_sys_fork

    .globl mini_sys_execve
    .type mini_sys_execve, @function
mini_sys_execve:
    movl $59, %eax
    syscall
    ret
    .size mini_sys_execve, .-mini_sys_execve

    .globl mini_sys_wait4
    .type mini_sys_wait4, @function
mini_sys_wait4:
    movq %rcx, %r10
    movl $61, %eax
    syscall
    ret
    .size mini_sys_wait4, .-mini_sys_wait4

    .globl mini_sys_getcwd
    .type mini_sys_getcwd, @function
mini_sys_getcwd:
    movl $79, %eax
    syscall
    ret
    .size mini_sys_getcwd, .-mini_sys_getcwd

    .globl mini_sys_chdir
    .type mini_sys_chdir, @function
mini_sys_chdir:
    movl $80, %eax
    syscall
    ret
    .size mini_sys_chdir, .-mini_sys_chdir

    .globl mini_sys_fchdir
    .type mini_sys_fchdir, @function
mini_sys_fchdir:
    movl $81, %eax
    syscall
    ret
    .size mini_sys_fchdir, .-mini_sys_fchdir

    .globl mini_sys_sched_yield
    .type mini_sys_sched_yield, @function
mini_sys_sched_yield:
    movl $24, %eax
    syscall
    ret
    .size mini_sys_sched_yield, .-mini_sys_sched_yield

    .globl mini_sys_nanosleep
    .type mini_sys_nanosleep, @function
mini_sys_nanosleep:
    movl $35, %eax
    syscall
    ret
    .size mini_sys_nanosleep, .-mini_sys_nanosleep

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

    .globl __mini_signal_restorer
    .hidden __mini_signal_restorer
    .type __mini_signal_restorer, @function
__mini_signal_restorer:
    leaq __mini_rt_sigreturn_restorer(%rip), %rax
    ret
    .size __mini_signal_restorer, .-__mini_signal_restorer

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

    .globl mini_sys_kill
    .type mini_sys_kill, @function
mini_sys_kill:
    movl $62, %eax
    syscall
    ret
    .size mini_sys_kill, .-mini_sys_kill

    .globl mini_sys_getppid
    .type mini_sys_getppid, @function
mini_sys_getppid:
    movl $110, %eax
    syscall
    ret
    .size mini_sys_getppid, .-mini_sys_getppid

    .globl mini_sys_setpgid
    .type mini_sys_setpgid, @function
mini_sys_setpgid:
    movl $109, %eax
    syscall
    ret
    .size mini_sys_setpgid, .-mini_sys_setpgid

    .globl mini_sys_getpgid
    .type mini_sys_getpgid, @function
mini_sys_getpgid:
    movl $121, %eax
    syscall
    ret
    .size mini_sys_getpgid, .-mini_sys_getpgid

    .globl mini_sys_setsid
    .type mini_sys_setsid, @function
mini_sys_setsid:
    movl $112, %eax
    syscall
    ret
    .size mini_sys_setsid, .-mini_sys_setsid

    .globl mini_sys_getsid
    .type mini_sys_getsid, @function
mini_sys_getsid:
    movl $124, %eax
    syscall
    ret
    .size mini_sys_getsid, .-mini_sys_getsid

    .globl mini_sys_arch_prctl
    .type mini_sys_arch_prctl, @function
mini_sys_arch_prctl:
    movl $158, %eax
    syscall
    ret
    .size mini_sys_arch_prctl, .-mini_sys_arch_prctl

    .globl mini_sys_gettid
    .type mini_sys_gettid, @function
mini_sys_gettid:
    movl $186, %eax
    syscall
    ret
    .size mini_sys_gettid, .-mini_sys_gettid

    .globl mini_sys_futex
    .type mini_sys_futex, @function
mini_sys_futex:
    movq %rcx, %r10
    movl $202, %eax
    syscall
    ret
    .size mini_sys_futex, .-mini_sys_futex

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

    .globl mini_sys_ftruncate
    .type mini_sys_ftruncate, @function
mini_sys_ftruncate:
    movl $77, %eax
    syscall
    ret
    .size mini_sys_ftruncate, .-mini_sys_ftruncate

    .globl mini_sys_getdents64
    .type mini_sys_getdents64, @function
mini_sys_getdents64:
    movl $217, %eax
    syscall
    ret
    .size mini_sys_getdents64, .-mini_sys_getdents64

    .globl mini_sys_openat
    .type mini_sys_openat, @function
mini_sys_openat:
    movq %rcx, %r10
    movl $257, %eax
    syscall
    ret
    .size mini_sys_openat, .-mini_sys_openat

    .globl mini_sys_newfstatat
    .type mini_sys_newfstatat, @function
mini_sys_newfstatat:
    movq %rcx, %r10
    movl $262, %eax
    syscall
    ret
    .size mini_sys_newfstatat, .-mini_sys_newfstatat

    .globl mini_sys_pipe2
    .type mini_sys_pipe2, @function
mini_sys_pipe2:
    movl $293, %eax
    syscall
    ret
    .size mini_sys_pipe2, .-mini_sys_pipe2

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

    .globl mini_sys_exit_group
    .type mini_sys_exit_group, @function
mini_sys_exit_group:
    movl $231, %eax
    syscall
    ud2
    .size mini_sys_exit_group, .-mini_sys_exit_group

    .section .note.GNU-stack,"",@progbits
