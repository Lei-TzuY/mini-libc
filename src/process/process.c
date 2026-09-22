#include <errno.h>
#include <mini/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

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
