#include <mini/syscall.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>

#define REGISTRY_WORKERS 6
#define REGISTRATIONS_PER_WORKER 5

static atomic_int registry_start = ATOMIC_VAR_INIT(0);
static atomic_int registry_errors = ATOMIC_VAR_INIT(0);
static atomic_int worker_callbacks = ATOMIC_VAR_INIT(0);
static atomic_int reentrant_callbacks = ATOMIC_VAR_INIT(0);
static atomic_int late_callbacks = ATOMIC_VAR_INIT(0);
static atomic_int last_ready = ATOMIC_VAR_INIT(0);
static atomic_int last_go = ATOMIC_VAR_INIT(0);

static int equals(const char *left, const char *right)
{
    while (*left != '\0' && *left == *right) {
        ++left;
        ++right;
    }
    return *left == *right;
}

static void write_marker(char marker)
{
    if (mini_sys_write(1, &marker, 1) != 1) {
        _Exit(90);
    }
}

static void normal_handler(void)
{
    write_marker('N');
}

static void quick_1(void)
{
    write_marker('1');
}

static void quick_2(void)
{
    write_marker('2');
}

static void abort_handler(int sig)
{
    if (sig != SIGABRT) {
        _Exit(91);
    }
    write_marker('H');
}

static void registry_worker_handler(void)
{
    (void)atomic_fetch_add(&worker_callbacks, 1);
}

static void registry_late_handler(void)
{
    (void)atomic_fetch_add(&late_callbacks, 1);
}

static void registry_reentrant_handler(void)
{
    (void)atomic_fetch_add(&reentrant_callbacks, 1);
    if (atexit(registry_late_handler) != 0) {
        atomic_store(&registry_errors, 1);
    }
}

static void registry_final_handler(void)
{
    if (atomic_load(&registry_errors) != 0 ||
        atomic_load(&worker_callbacks) !=
            REGISTRY_WORKERS * REGISTRATIONS_PER_WORKER ||
        atomic_load(&reentrant_callbacks) != 1 ||
        atomic_load(&late_callbacks) != 1) {
        write_marker('X');
        return;
    }
    write_marker('R');
}

static int registry_worker(void *opaque)
{
    int index;

    (void)opaque;
    while (atomic_load(&registry_start) == 0) {
        thrd_yield();
    }
    for (index = 0; index < REGISTRATIONS_PER_WORKER; ++index) {
        if (atexit(registry_worker_handler) != 0) {
            atomic_store(&registry_errors, 1);
            return -1;
        }
        thrd_yield();
    }
    return 0;
}

static _Noreturn void run_registry(void)
{
    thrd_t workers[REGISTRY_WORKERS];
    int index;

    if (atexit(registry_final_handler) != 0) {
        _Exit(94);
    }
    for (index = 0; index < REGISTRY_WORKERS; ++index) {
        if (thrd_create(&workers[index], registry_worker, (void *)0) !=
            thrd_success) {
            _Exit(95);
        }
    }
    atomic_store(&registry_start, 1);
    for (index = 0; index < REGISTRY_WORKERS; ++index) {
        int result;

        if (thrd_join(workers[index], &result) != thrd_success || result != 0) {
            _Exit(96);
        }
    }
    if (atomic_load(&registry_errors) != 0 ||
        atexit(registry_reentrant_handler) != 0 ||
        atexit(registry_worker_handler) == 0) {
        _Exit(97);
    }
    exit(EXIT_SUCCESS);
}

static void last_handler(void)
{
    if (fputc('H', stdout) != 'H') {
        _Exit(98);
    }
}

static int detached_last_worker(void *opaque)
{
    (void)opaque;
    atomic_store(&last_ready, 1);
    while (atomic_load(&last_go) == 0) {
        thrd_yield();
    }
    return 77;
}

static _Noreturn void run_last_thread(void)
{
    thrd_t worker;

    if (atexit(last_handler) != 0 || fputc('B', stdout) != 'B' ||
        thrd_create(&worker, detached_last_worker, (void *)0) != thrd_success) {
        _Exit(99);
    }
    while (atomic_load(&last_ready) == 0) {
        thrd_yield();
    }
    if (thrd_detach(worker) != thrd_success) {
        _Exit(100);
    }
    atomic_store(&last_go, 1);
    thrd_exit(42);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        return 1;
    }

    if (equals(argv[1], "quick")) {
        if (atexit(normal_handler) != 0 || at_quick_exit(quick_1) != 0 ||
            at_quick_exit(quick_2) != 0 || fputc('Z', stdout) != 'Z') {
            _Exit(92);
        }
        quick_exit(41);
    }

    if (equals(argv[1], "abort")) {
        if (atexit(normal_handler) != 0 || at_quick_exit(quick_1) != 0 ||
            signal(SIGABRT, abort_handler) == SIG_ERR ||
            fputc('Z', stdout) != 'Z') {
            _Exit(93);
        }
        abort();
    }

    if (equals(argv[1], "registry")) {
        run_registry();
    }

    if (equals(argv[1], "last-thread")) {
        run_last_thread();
    }

    return 2;
}
