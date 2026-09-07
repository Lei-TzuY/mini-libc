#include <mini/syscall.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>

#define REGISTRY_WORKERS 10
#define REGISTRATIONS_PER_WORKER 3

static atomic_int registry_start = ATOMIC_VAR_INIT(0);
static atomic_int registry_errors = ATOMIC_VAR_INIT(0);
static atomic_int worker_callbacks = ATOMIC_VAR_INIT(0);
static atomic_int reentrant_callbacks = ATOMIC_VAR_INIT(0);
static atomic_int late_callbacks = ATOMIC_VAR_INIT(0);
static int registry_quick;

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

static void raw_marker(const char *text)
{
    const char *end = text;

    while (*end != '\0') {
        ++end;
    }
    if (mini_sys_write(1, text, (unsigned long)(end - text)) !=
        (long)(end - text)) {
        _Exit(90);
    }
}

static int register_callback(void (*func)(void))
{
    return registry_quick ? at_quick_exit(func) : atexit(func);
}

static void worker_callback(void)
{
    (void)atomic_fetch_add(&worker_callbacks, 1);
}

static void late_callback(void)
{
    (void)atomic_fetch_add(&late_callbacks, 1);
}

static void reentrant_callback(void)
{
    (void)atomic_fetch_add(&reentrant_callbacks, 1);
    if (register_callback(late_callback) != 0) {
        atomic_store(&registry_errors, 1);
    }
}

static void final_checker(void)
{
    if (atomic_load(&registry_errors) != 0 ||
        atomic_load(&worker_callbacks) !=
            REGISTRY_WORKERS * REGISTRATIONS_PER_WORKER ||
        atomic_load(&reentrant_callbacks) != 1 ||
        atomic_load(&late_callbacks) != 1) {
        raw_marker("registry-bad");
        return;
    }
    raw_marker(registry_quick ? "quick-registry-ok" : "normal-registry-ok");
}

static int registry_worker(void *opaque)
{
    int index;

    (void)opaque;
    while (atomic_load(&registry_start) == 0) {
        thrd_yield();
    }
    for (index = 0; index < REGISTRATIONS_PER_WORKER; ++index) {
        if (register_callback(worker_callback) != 0) {
            atomic_store(&registry_errors, 1);
            return -1;
        }
        thrd_yield();
    }
    return 0;
}

static _Noreturn void run_registry(int quick)
{
    thrd_t workers[REGISTRY_WORKERS];
    int index;

    registry_quick = quick;
    if (register_callback(final_checker) != 0) {
        _Exit(91);
    }
    for (index = 0; index < REGISTRY_WORKERS; ++index) {
        if (thrd_create(&workers[index], registry_worker, (void *)0) !=
            thrd_success) {
            _Exit(92);
        }
    }
    atomic_store(&registry_start, 1);
    for (index = 0; index < REGISTRY_WORKERS; ++index) {
        int result;

        if (thrd_join(workers[index], &result) != thrd_success || result != 0) {
            _Exit(93);
        }
    }
    if (atomic_load(&registry_errors) != 0 ||
        register_callback(reentrant_callback) != 0 ||
        register_callback(worker_callback) == 0) {
        _Exit(94);
    }

    if (quick) {
        quick_exit(EXIT_SUCCESS);
    }
    exit(EXIT_SUCCESS);
}

static void last_handler(void)
{
    if (fputc('H', stdout) != 'H') {
        _Exit(95);
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
        _Exit(96);
    }
    while (atomic_load(&last_ready) == 0) {
        thrd_yield();
    }
    if (thrd_detach(worker) != thrd_success) {
        _Exit(97);
    }
    atomic_store(&last_go, 1);
    thrd_exit(42);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        return 1;
    }
    if (equals(argv[1], "normal-registry")) {
        run_registry(0);
    }
    if (equals(argv[1], "quick-registry")) {
        run_registry(1);
    }
    if (equals(argv[1], "last-thread")) {
        run_last_thread();
    }
    return 2;
}
