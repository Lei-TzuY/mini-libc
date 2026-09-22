#include <errno.h>
#include <mini/syscall.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <threads.h>
#include <unistd.h>

struct worker_state {
    mtx_t mutex;
    atomic_int ready;
    atomic_int release;
};

struct child_report {
    int count;
    int trace[8];
};

static int trace_values[8];
static int trace_count;

static void record_value(int value)
{
    if (trace_count < 8) {
        trace_values[trace_count] = value;
        ++trace_count;
    }
}

static void prepare_a(void)
{
    record_value(1);
}

static void parent_a(void)
{
    record_value(2);
}

static void child_a(void)
{
    record_value(3);
}

static void prepare_b(void)
{
    record_value(4);
}

static void parent_b(void)
{
    record_value(5);
}

static void child_b(void)
{
    record_value(6);
}

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

static int trace_is(const int *expected, int count)
{
    int index;

    if (trace_count != count) {
        return 0;
    }
    for (index = 0; index < count; ++index) {
        if (trace_values[index] != expected[index]) {
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    static const char ok[] = "atfork-coordination-ok\n";
    static const int parent_expected[4] = {4, 1, 2, 5};
    static const int child_expected[4] = {4, 1, 3, 6};
    struct worker_state state;
    struct child_report report;
    int report_pipe[2];
    int raw_pipe[2];
    int raw_count;
    int worker_result;
    int status;
    int index;
    pid_t child;
    pid_t raw_child;
    thrd_t thread;

    errno = ERANGE;
    if (pthread_atfork(prepare_a, parent_a, child_a) != 0 ||
        errno != ERANGE ||
        pthread_atfork(prepare_b, parent_b, child_b) != 0 ||
        errno != ERANGE) {
        return 1;
    }

    for (index = 0; index < 6; ++index) {
        if (pthread_atfork((void (*)(void))0,
                           (void (*)(void))0,
                           (void (*)(void))0) != 0 ||
            errno != ERANGE) {
            return 2;
        }
    }
    if (pthread_atfork((void (*)(void))0,
                       (void (*)(void))0,
                       (void (*)(void))0) != ENOMEM ||
        errno != ERANGE) {
        return 3;
    }

    if (mtx_init(&state.mutex, mtx_plain) != thrd_success) {
        return 4;
    }
    atomic_init(&state.ready, 0);
    atomic_init(&state.release, 0);
    if (thrd_create(&thread, worker, &state) != thrd_success) {
        mtx_destroy(&state.mutex);
        return 5;
    }
    while (atomic_load(&state.ready) == 0) {
        thrd_yield();
    }
    if (mtx_trylock(&state.mutex) != thrd_busy) {
        atomic_store(&state.release, 1);
        (void)thrd_join(thread, &worker_result);
        mtx_destroy(&state.mutex);
        return 6;
    }

    errno = ERANGE;
    if (pipe(report_pipe) != 0 || errno != ERANGE) {
        atomic_store(&state.release, 1);
        (void)thrd_join(thread, &worker_result);
        mtx_destroy(&state.mutex);
        return 7;
    }

    trace_count = 0;
    errno = ERANGE;
    child = fork();
    if (child < 0) {
        close(report_pipe[0]);
        close(report_pipe[1]);
        atomic_store(&state.release, 1);
        (void)thrd_join(thread, &worker_result);
        mtx_destroy(&state.mutex);
        return 8;
    }

    if (child == 0) {
        if (!trace_is(child_expected, 4) || errno != ERANGE) {
            _Exit(90);
        }
        report.count = trace_count;
        for (index = 0; index < trace_count; ++index) {
            report.trace[index] = trace_values[index];
        }
        if (close(report_pipe[0]) != 0 ||
            write(report_pipe[1], &report, sizeof(report)) !=
                (ssize_t)sizeof(report) ||
            close(report_pipe[1]) != 0) {
            _Exit(91);
        }
        _Exit(23);
    }

    if (!trace_is(parent_expected, 4) || errno != ERANGE) {
        (void)kill(child, SIGTERM);
        (void)waitpid(child, &status, 0);
        return 9;
    }

    atomic_store(&state.release, 1);
    if (thrd_join(thread, &worker_result) != thrd_success ||
        worker_result != 72) {
        return 10;
    }
    mtx_destroy(&state.mutex);

    if (close(report_pipe[1]) != 0 ||
        read(report_pipe[0], &report, sizeof(report)) !=
            (ssize_t)sizeof(report) ||
        close(report_pipe[0]) != 0) {
        return 11;
    }
    if (report.count != 4 ||
        report.trace[0] != 4 || report.trace[1] != 1 ||
        report.trace[2] != 3 || report.trace[3] != 6) {
        return 12;
    }

    status = -1;
    errno = ERANGE;
    if (waitpid(child, &status, 0) != child || errno != ERANGE ||
        !WIFEXITED(status) || WEXITSTATUS(status) != 23) {
        return 13;
    }

    errno = ERANGE;
    if (pipe(raw_pipe) != 0 || errno != ERANGE) {
        return 14;
    }
    trace_count = 0;
    raw_child = _Fork();
    if (raw_child < 0) {
        close(raw_pipe[0]);
        close(raw_pipe[1]);
        return 15;
    }

    if (raw_child == 0) {
        raw_count = trace_count;
        if (close(raw_pipe[0]) != 0 ||
            write(raw_pipe[1], &raw_count, sizeof(raw_count)) !=
                (ssize_t)sizeof(raw_count) ||
            close(raw_pipe[1]) != 0) {
            _Exit(92);
        }
        _Exit(24);
    }

    if (trace_count != 0 || close(raw_pipe[1]) != 0 ||
        read(raw_pipe[0], &raw_count, sizeof(raw_count)) !=
            (ssize_t)sizeof(raw_count) ||
        close(raw_pipe[0]) != 0 ||
        raw_count != 0) {
        return 16;
    }

    status = -1;
    errno = ERANGE;
    if (waitpid(raw_child, &status, 0) != raw_child || errno != ERANGE ||
        !WIFEXITED(status) || WEXITSTATUS(status) != 24) {
        return 17;
    }

    if (mini_sys_write(STDOUT_FILENO, ok, sizeof(ok) - 1U) !=
        (long)(sizeof(ok) - 1U)) {
        return 18;
    }
    return 0;
}
