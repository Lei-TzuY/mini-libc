#ifndef MINI_LIBC_SPAWN_H
#define MINI_LIBC_SPAWN_H

#include <sys/types.h>

#define __MINI_POSIX_SPAWN_ACTION_CAPACITY 8U

struct __mini_posix_spawn_action {
    int __kind;
    int __fd;
    int __newfd;
};

typedef struct {
    unsigned int __magic;
    unsigned int __count;
    struct __mini_posix_spawn_action
        __actions[__MINI_POSIX_SPAWN_ACTION_CAPACITY];
} posix_spawn_file_actions_t;

struct __mini_posix_spawnattr;
typedef struct __mini_posix_spawnattr posix_spawnattr_t;

int posix_spawn_file_actions_init(posix_spawn_file_actions_t *file_actions);
int posix_spawn_file_actions_destroy(posix_spawn_file_actions_t *file_actions);
int posix_spawn_file_actions_addclose(posix_spawn_file_actions_t *file_actions,
                                      int fd);
int posix_spawn_file_actions_adddup2(posix_spawn_file_actions_t *file_actions,
                                     int fd, int newfd);

int posix_spawn(pid_t *restrict pid, const char *restrict path,
                const posix_spawn_file_actions_t *file_actions,
                const posix_spawnattr_t *restrict attrp,
                char *const argv[restrict],
                char *const envp[restrict]);

#endif
