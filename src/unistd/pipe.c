#include <errno.h>
#include <mini/syscall.h>
#include <unistd.h>

int pipe2(int pipefd[2], int flags)
{
    long result = mini_sys_pipe2(pipefd, flags);

    if (result < 0L) {
        errno = (int)-result;
        return -1;
    }
    return 0;
}

int pipe(int pipefd[2])
{
    return pipe2(pipefd, 0);
}
