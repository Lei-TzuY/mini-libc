#include <mini/syscall.h>

extern int main(int argc, char **argv, char **envp);
extern void __mini_set_envp(char **envp);

_Noreturn void __mini_start(long *initial_stack)
{
    long raw_argc = initial_stack[0];
    int argc = (int)raw_argc;
    char **argv = (char **)&initial_stack[1];
    char **envp = argv + raw_argc + 1;
    int status;

    __mini_set_envp(envp);
    status = main(argc, argv, envp);

    mini_sys_exit(status);
}
