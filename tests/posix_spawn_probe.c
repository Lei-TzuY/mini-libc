#include <errno.h>
#include <fcntl.h>
#include <mini/syscall.h>
#include <poll.h>
#include <spawn.h>
#include <stdatomic.h>
#include <stddef.h>
#include <string.h>
#include <sys/wait.h>
#include <threads.h>
#include <unistd.h>

struct worker_state {
    mtx_t mutex;
    atomic_int ready;
    atomic_int release;
};

static int worker(void *opaque)
{
    struct worker_state *state = (struct worker_state *)opaque;

    if (mtx_lock(&state->mutex) != thrd_success) {
        return 70;
    }
    atomic_store(&state->ready, 1);
    while (atomic_load(&state->release) == 0) {
        thrd_yield();
    }
    if (mtx_unlock(&state->mutex) != thrd_success) {
        return 71;
    }
    return 72;
}

static int same_bytes(const char *left, const char *right, size_t count)
{
    size_t index;

    for (index = 0U; index < count; ++index) {
        if (left[index] != right[index]) {
            return 0;
        }
    }
    return 1;
}

int main(int argc, char **argv)
{
    static const char expected[] = "spawn-child-ok\n";
    static char child_name[] = "spawn-child";
    static char arg1[] = "gamma";
    static char arg2[] = "delta";
    static char token[] = "MINI_SPAWN_TOKEN=threaded-ok";
    static char missing[] = "mini-libc-spawn-definitely-missing";
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_t destroyed;
    struct worker_state state;
    struct pollfd ready;
    char *child_argv[4];
    char *child_envp[2];
    char *missing_argv[2];
    char *empty_envp[1];
    char buffer[32];
    int pipefd[2];
    int worker_result;
    int status;
    int result;
    int count;
    pid_t child;
    pid_t missing_child;
    thrd_t thread;

    if (argc != 2) {
        return 1;
    }

    errno = ERANGE;
    if (posix_spawn_file_actions_init(&actions) != 0 || errno != ERANGE ||
        posix_spawn_file_actions_addclose(&actions, -1) != EBADF ||
        errno != ERANGE ||
        posix_spawn_file_actions_adddup2(&actions, -1, STDOUT_FILENO) != EBADF ||
        errno != ERANGE) {
        return 2;
    }

    errno = ERANGE;
    if (pipe2(pipefd, O_CLOEXEC) != 0 || errno != ERANGE ||
        posix_spawn_file_actions_addclose(&actions, pipefd[0]) != 0 ||
        errno != ERANGE ||
        posix_spawn_file_actions_adddup2(&actions, pipefd[1],
                                         STDOUT_FILENO) != 0 ||
        errno != ERANGE ||
        posix_spawn_file_actions_addclose(&actions, pipefd[1]) != 0 ||
        errno != ERANGE) {
        if (pipefd[0] >= 0) {
            close(pipefd[0]);
        }
        if (pipefd[1] >= 0) {
            close(pipefd[1]);
        }
        return 3;
    }

    if (mtx_init(&state.mutex, mtx_plain) != thrd_success) {
        close(pipefd[0]);
        close(pipefd[1]);
        return 4;
    }
    atomic_init(&state.ready, 0);
    atomic_init(&state.release, 0);
    if (thrd_create(&thread, worker, &state) != thrd_success) {
        mtx_destroy(&state.mutex);
        close(pipefd[0]);
        close(pipefd[1]);
        return 5;
    }
    while (atomic_load(&state.ready) == 0) {
        thrd_yield();
    }
    if (mtx_trylock(&state.mutex) != thrd_busy) {
        atomic_store(&state.release, 1);
        (void)thrd_join(thread, &worker_result);
        mtx_destroy(&state.mutex);
        close(pipefd[0]);
        close(pipefd[1]);
        return 6;
    }

    child_argv[0] = child_name;
    child_argv[1] = arg1;
    child_argv[2] = arg2;
    child_argv[3] = (char *)0;
    child_envp[0] = token;
    child_envp[1] = (char *)0;

    errno = ERANGE;
    result = posix_spawn(&child, argv[1], &actions,
                         (const posix_spawnattr_t *)0,
                         child_argv, child_envp);
    if (result != 0 || errno != ERANGE || child <= 0) {
        atomic_store(&state.release, 1);
        (void)thrd_join(thread, &worker_result);
        mtx_destroy(&state.mutex);
        close(pipefd[0]);
        close(pipefd[1]);
        return 7;
    }

    atomic_store(&state.release, 1);
    if (thrd_join(thread, &worker_result) != thrd_success ||
        worker_result != 72) {
        mtx_destroy(&state.mutex);
        close(pipefd[0]);
        close(pipefd[1]);
        return 8;
    }
    mtx_destroy(&state.mutex);

    errno = ERANGE;
    if (close(pipefd[1]) != 0 || errno != ERANGE) {
        close(pipefd[0]);
        return 9;
    }

    ready.fd = pipefd[0];
    ready.events = POLLIN;
    ready.revents = 0;
    errno = ERANGE;
    if (poll(&ready, 1U, -1) != 1 || errno != ERANGE ||
        (ready.revents & POLLIN) == 0) {
        close(pipefd[0]);
        return 10;
    }

    errno = ERANGE;
    count = (int)read(pipefd[0], buffer, sizeof(buffer));
    if (count != (int)(sizeof(expected) - 1U) ||
        errno != ERANGE ||
        !same_bytes(buffer, expected, sizeof(expected) - 1U)) {
        close(pipefd[0]);
        return 11;
    }

    status = -1;
    errno = ERANGE;
    if (waitpid(child, &status, 0) != child || errno != ERANGE ||
        !WIFEXITED(status) || WEXITSTATUS(status) != 44) {
        close(pipefd[0]);
        return 12;
    }

    errno = ERANGE;
    if (read(pipefd[0], buffer, 1U) != 0 || errno != ERANGE ||
        close(pipefd[0]) != 0 || errno != ERANGE) {
        return 13;
    }

    errno = ERANGE;
    if (posix_spawn_file_actions_destroy(&actions) != 0 || errno != ERANGE) {
        return 14;
    }

    errno = ERANGE;
    if (posix_spawn_file_actions_init(&destroyed) != 0 ||
        posix_spawn_file_actions_destroy(&destroyed) != 0 ||
        posix_spawn(&child, argv[1], &destroyed,
                    (const posix_spawnattr_t *)0,
                    child_argv, child_envp) != EINVAL ||
        errno != ERANGE) {
        return 15;
    }

    missing_argv[0] = missing;
    missing_argv[1] = (char *)0;
    empty_envp[0] = (char *)0;
    errno = ERANGE;
    result = posix_spawn(&missing_child, missing,
                         (const posix_spawn_file_actions_t *)0,
                         (const posix_spawnattr_t *)0,
                         missing_argv, empty_envp);
    if (result != 0 || errno != ERANGE || missing_child <= 0) {
        return 16;
    }
    status = -1;
    errno = ERANGE;
    if (waitpid(missing_child, &status, 0) != missing_child ||
        errno != ERANGE || !WIFEXITED(status) ||
        WEXITSTATUS(status) != 127) {
        return 17;
    }

    if (mini_sys_write(STDOUT_FILENO, "posix-spawn-ok\n", 15U) != 15L) {
        return 18;
    }
    return 0;
}
