#include <mini/syscall.h>

#define MINI_ATEXIT_CAPACITY 32U

extern int main(int argc, char **argv, char **envp);
extern void __mini_set_envp(char **envp);

static void (*mini_atexit_handlers[MINI_ATEXIT_CAPACITY])(void);
static unsigned int mini_atexit_count;

int atexit(void (*func)(void))
{
    if (func == (void (*)(void))0 || mini_atexit_count == MINI_ATEXIT_CAPACITY) {
        return -1;
    }

    mini_atexit_handlers[mini_atexit_count] = func;
    ++mini_atexit_count;
    return 0;
}

_Noreturn void _Exit(int status)
{
    mini_sys_exit(status);
}

_Noreturn void exit(int status)
{
    while (mini_atexit_count != 0U) {
        void (*handler)(void);

        --mini_atexit_count;
        handler = mini_atexit_handlers[mini_atexit_count];
        mini_atexit_handlers[mini_atexit_count] = (void (*)(void))0;
        handler();
    }

    _Exit(status);
}

_Noreturn void __mini_start(long *initial_stack)
{
    long raw_argc = initial_stack[0];
    int argc = (int)raw_argc;
    char **argv = (char **)&initial_stack[1];
    char **envp = argv + raw_argc + 1;
    int status;

    __mini_set_envp(envp);
    status = main(argc, argv, envp);

    exit(status);
}
