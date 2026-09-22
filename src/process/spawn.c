#include <errno.h>
#include <mini/syscall.h>
#include <spawn.h>
#include <unistd.h>

#define MINI_SPAWN_ACTIONS_MAGIC 0x53504157U
#define MINI_SPAWN_ACTION_CLOSE 1
#define MINI_SPAWN_ACTION_DUP2 2
#define MINI_SPAWN_CHILD_FAILURE 127

static int actions_valid(const posix_spawn_file_actions_t *file_actions)
{
    return file_actions != (const posix_spawn_file_actions_t *)0 &&
           file_actions->__magic == MINI_SPAWN_ACTIONS_MAGIC &&
           file_actions->__count <= __MINI_POSIX_SPAWN_ACTION_CAPACITY;
}

int posix_spawn_file_actions_init(posix_spawn_file_actions_t *file_actions)
{
    int saved_errno = errno;

    if (file_actions == (posix_spawn_file_actions_t *)0) {
        errno = saved_errno;
        return EINVAL;
    }
    file_actions->__magic = MINI_SPAWN_ACTIONS_MAGIC;
    file_actions->__count = 0U;
    errno = saved_errno;
    return 0;
}

int posix_spawn_file_actions_destroy(posix_spawn_file_actions_t *file_actions)
{
    int saved_errno = errno;

    if (!actions_valid(file_actions)) {
        errno = saved_errno;
        return EINVAL;
    }
    file_actions->__magic = 0U;
    file_actions->__count = 0U;
    errno = saved_errno;
    return 0;
}

static int reserve_action(posix_spawn_file_actions_t *file_actions,
                          int kind, int fd, int newfd)
{
    unsigned int index;

    if (!actions_valid(file_actions)) {
        return EINVAL;
    }
    if (file_actions->__count == __MINI_POSIX_SPAWN_ACTION_CAPACITY) {
        return ENOMEM;
    }
    index = file_actions->__count;
    file_actions->__actions[index].__kind = kind;
    file_actions->__actions[index].__fd = fd;
    file_actions->__actions[index].__newfd = newfd;
    file_actions->__count = index + 1U;
    return 0;
}

int posix_spawn_file_actions_addclose(posix_spawn_file_actions_t *file_actions,
                                      int fd)
{
    int saved_errno = errno;
    int result;

    if (fd < 0) {
        errno = saved_errno;
        return EBADF;
    }
    result = reserve_action(file_actions, MINI_SPAWN_ACTION_CLOSE, fd, -1);
    errno = saved_errno;
    return result;
}

int posix_spawn_file_actions_adddup2(posix_spawn_file_actions_t *file_actions,
                                     int fd, int newfd)
{
    int saved_errno = errno;
    int result;

    if (fd < 0 || newfd < 0) {
        errno = saved_errno;
        return EBADF;
    }
    result = reserve_action(file_actions, MINI_SPAWN_ACTION_DUP2, fd, newfd);
    errno = saved_errno;
    return result;
}

static _Noreturn void spawn_child(const char *path,
                                  const posix_spawn_file_actions_t *file_actions,
                                  char *const argv[], char *const envp[])
{
    unsigned int index;

    if (file_actions != (const posix_spawn_file_actions_t *)0) {
        for (index = 0U; index < file_actions->__count; ++index) {
            const struct __mini_posix_spawn_action *action =
                &file_actions->__actions[index];
            long result;

            if (action->__kind == MINI_SPAWN_ACTION_CLOSE) {
                result = mini_sys_close(action->__fd);
            } else if (action->__kind == MINI_SPAWN_ACTION_DUP2) {
                result = mini_sys_dup2(action->__fd, action->__newfd);
            } else {
                mini_sys_exit(MINI_SPAWN_CHILD_FAILURE);
            }
            if (result < 0L) {
                mini_sys_exit(MINI_SPAWN_CHILD_FAILURE);
            }
        }
    }

    (void)mini_sys_execve(path, argv, envp);
    mini_sys_exit(MINI_SPAWN_CHILD_FAILURE);
}

int posix_spawn(pid_t *restrict pid, const char *restrict path,
                const posix_spawn_file_actions_t *file_actions,
                const posix_spawnattr_t *restrict attrp,
                char *const argv[restrict], char *const envp[restrict])
{
    int saved_errno = errno;
    long child;

    if (path == (const char *)0 || argv == (char *const *)0 ||
        envp == (char *const *)0 || attrp != (const posix_spawnattr_t *)0 ||
        (file_actions != (const posix_spawn_file_actions_t *)0 &&
         !actions_valid(file_actions))) {
        errno = saved_errno;
        return EINVAL;
    }

    child = mini_sys_fork();
    if (child < 0L) {
        errno = saved_errno;
        return (int)-child;
    }
    if (child == 0L) {
        spawn_child(path, file_actions, argv, envp);
    }

    if (pid != (pid_t *)0) {
        *pid = (pid_t)child;
    }
    errno = saved_errno;
    return 0;
}
