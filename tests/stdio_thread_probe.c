#include <stdio.h>
#include <string.h>
#include <threads.h>

#define WORKER_COUNT 6
#define RECORDS_PER_WORKER 64
#define TOTAL_RECORDS (WORKER_COUNT * RECORDS_PER_WORKER)

struct writer_arg {
    FILE *stream;
    int id;
};

static mtx_t gate_lock;
static cnd_t ready_condition;
static cnd_t release_condition;
static int ready_workers;
static int release_workers;

static unsigned int record_value(int id, int record)
{
    return 0xa5000000U | ((unsigned int)id << 12) |
           (unsigned int)record;
}

static int writer(void *opaque)
{
    struct writer_arg *arg = (struct writer_arg *)opaque;
    int record;

    if (mtx_lock(&gate_lock) != thrd_success) {
        return 101;
    }
    ++ready_workers;
    if (cnd_signal(&ready_condition) != thrd_success) {
        (void)mtx_unlock(&gate_lock);
        return 102;
    }
    while (!release_workers) {
        if (cnd_wait(&release_condition, &gate_lock) != thrd_success) {
            (void)mtx_unlock(&gate_lock);
            return 103;
        }
    }
    if (mtx_unlock(&gate_lock) != thrd_success) {
        return 104;
    }

    for (record = 0; record < RECORDS_PER_WORKER; ++record) {
        if (fprintf(arg->stream,
                    "T%d:%02d:%08x:ABCDEFGHIJKLMNOPQRSTUVWXYZ:%d:%d\n",
                    arg->id, record, record_value(arg->id, record),
                    arg->id + record, arg->id - record) <= 0) {
            return 105;
        }
    }
    return 0;
}

static int validate_records(FILE *stream)
{
    static unsigned char seen[WORKER_COUNT][RECORDS_PER_WORKER];
    static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int count;

    for (count = 0; count < TOTAL_RECORDS; ++count) {
        char text[27];
        unsigned int value;
        int id;
        int record;
        int sum;
        int difference;

        if (fscanf(stream, " T%d:%d:%x:%26[A-Z]:%d:%d",
                   &id, &record, &value, text, &sum, &difference) != 6) {
            return 0;
        }
        if (id < 0 || id >= WORKER_COUNT || record < 0 ||
            record >= RECORDS_PER_WORKER || seen[id][record] ||
            value != record_value(id, record) ||
            strcmp(text, alphabet) != 0 || sum != id + record ||
            difference != id - record) {
            return 0;
        }
        seen[id][record] = 1U;
    }

    {
        int id;
        int record;

        for (id = 0; id < WORKER_COUNT; ++id) {
            for (record = 0; record < RECORDS_PER_WORKER; ++record) {
                if (!seen[id][record]) {
                    return 0;
                }
            }
        }
    }

    {
        int extra;

        if (fscanf(stream, " %d", &extra) != EOF) {
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    static const char marker[] = "stdio-thread-ok";
    struct writer_arg args[WORKER_COUNT];
    thrd_t threads[WORKER_COUNT];
    FILE *stream;
    int i;

    if (mtx_init(&gate_lock, mtx_plain) != thrd_success ||
        cnd_init(&ready_condition) != thrd_success ||
        cnd_init(&release_condition) != thrd_success) {
        return 1;
    }

    stream = tmpfile();
    if (stream == (FILE *)0) {
        return 2;
    }

    for (i = 0; i < WORKER_COUNT; ++i) {
        args[i].stream = stream;
        args[i].id = i;
        if (thrd_create(&threads[i], writer, &args[i]) != thrd_success) {
            return 3;
        }
    }

    if (mtx_lock(&gate_lock) != thrd_success) {
        return 4;
    }
    while (ready_workers != WORKER_COUNT) {
        if (cnd_wait(&ready_condition, &gate_lock) != thrd_success) {
            return 5;
        }
    }
    release_workers = 1;
    if (cnd_broadcast(&release_condition) != thrd_success ||
        mtx_unlock(&gate_lock) != thrd_success) {
        return 6;
    }

    for (i = 0; i < WORKER_COUNT; ++i) {
        int result;

        if (thrd_join(threads[i], &result) != thrd_success || result != 0) {
            return 7;
        }
    }

    if (fflush(stream) == EOF || fseek(stream, 0L, SEEK_SET) != 0 ||
        !validate_records(stream) || fclose(stream) == EOF) {
        return 8;
    }

    cnd_destroy(&release_condition);
    cnd_destroy(&ready_condition);
    mtx_destroy(&gate_lock);
    if (fputs(marker, stdout) == EOF || fflush(stdout) == EOF) {
        return 9;
    }
    return 0;
}
