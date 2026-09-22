#ifndef MINI_LIBC_SYS_RESOURCE_H
#define MINI_LIBC_SYS_RESOURCE_H

typedef unsigned long rlim_t;

struct rlimit {
    rlim_t rlim_cur;
    rlim_t rlim_max;
};

#define RLIMIT_FSIZE 1
#define RLIM_INFINITY (~0UL)

int getrlimit(int resource, struct rlimit *rlim);
int setrlimit(int resource, const struct rlimit *rlim);

#endif
