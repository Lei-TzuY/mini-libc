#ifndef MINI_LIBC_SIGNAL_H
#define MINI_LIBC_SIGNAL_H

#include <sys/types.h>

typedef int sig_atomic_t;

#define SIG_DFL ((void (*)(int))0)
#define SIG_IGN ((void (*)(int))1)
#define SIG_ERR ((void (*)(int))-1)

#define SIGINT 2
#define SIGILL 4
#define SIGABRT 6
#define SIGFPE 8
#define SIGSEGV 11
#define SIGPIPE 13
#define SIGTERM 15
#define SIGCONT 18
#define SIGTSTP 20
#define SIGTTOU 22
#define SIGXFSZ 25

void (*signal(int sig, void (*func)(int)))(int);
int raise(int sig);
int kill(pid_t pid, int sig);

#endif
