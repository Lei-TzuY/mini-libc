#include <errno.h>
#include <mini/syscall.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

pid_t getpid(void)
{
    return (pid_t)mini_sys_getpid();
}

pid_t getppid(void)
{
    return (pid_t)mini_sys_getppid();
}

int kill(pid_t pid, int sig)
{
    int saved_errno = errno;
    long result = mini_sys_kill((int)pid, sig);

    if (result < 0L) {
        errno = (int)-result;
        return -1;
    }
    errno = saved_errno;
    return 0;
}

pid_t getpgid(pid_t pid)
{
    int saved_errno = errno;
    long result = mini_sys_getpgid((int)pid);

    if (result < 0L) {
        errno = (int)-result;
        return (pid_t)-1;
    }
    errno = saved_errno;
    return (pid_t)result;
}

pid_t getpgrp(void)
{
    return getpgid((pid_t)0);
}

int setpgid(pid_t pid, pid_t pgid)
{
    int saved_errno = errno;
    long result = mini_sys_setpgid((int)pid, (int)pgid);

    if (result < 0L) {
        errno = (int)-result;
        return -1;
    }
    errno = saved_errno;
    return 0;
}

pid_t getsid(pid_t pid)
{
    int saved_errno = errno;
    long result = mini_sys_getsid((int)pid);

    if (result < 0L) {
        errno = (int)-result;
        return (pid_t)-1;
    }
    errno = saved_errno;
    return (pid_t)result;
}

pid_t setsid(void)
{
    int saved_errno = errno;
    long result = mini_sys_setsid();

    if (result < 0L) {
        errno = (int)-result;
        return (pid_t)-1;
    }
    errno = saved_errno;
    return (pid_t)result;
}

pid_t fork(void)
{
    long result = mini_sys_fork();

    if (result < 0L) {
        errno = (int)-result;
        return (pid_t)-1;
    }
    return (pid_t)result;
}

pid_t waitpid(pid_t pid, int *status, int options)
{
    long result = mini_sys_wait4((int)pid, status, options, (void *)0);

    if (result < 0L) {
        errno = (int)-result;
        return (pid_t)-1;
    }
    return (pid_t)result;
}

int execve(const char *path, char *const argv[], char *const envp[])
{
    long result = mini_sys_execve(path, argv, envp);

    errno = (int)-result;
    return -1;
}
