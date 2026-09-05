#include <stdlib.h>

extern int main(int argc, char **argv, char **envp);
extern void __mini_set_envp(char **envp);
extern void __mini_thread_runtime_init_main(void);

_Noreturn void __mini_start(long *initial_stack)
{
    long raw_argc = initial_stack[0];
    int argc = (int)raw_argc;
    char **argv = (char **)&initial_stack[1];
    char **envp = argv + raw_argc + 1;
    int status;

    __mini_thread_runtime_init_main();
    __mini_set_envp(envp);
    status = main(argc, argv, envp);

    exit(status);
}
