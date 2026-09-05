#include <errno.h>
#include <stddef.h>
#include <stdio.h>

#define TEST_AT_FDCWD (-100)
#define TEST_O_RDONLY 0
#define TEST_O_WRONLY 1
#define TEST_O_RDWR 2
#define TEST_O_CREAT 64
#define TEST_O_TRUNC 512

struct write_step {
    int fd;
    long result;
};

struct open_step {
    const char *path;
    int flags;
    long result;
};

struct close_step {
    int fd;
    long result;
};

static struct write_step writes[16];
static struct open_step opens[16];
static struct close_step closes[16];
static size_t write_count;
static size_t write_index;
static size_t open_count;
static size_t open_index;
static size_t close_count;
static size_t close_index;
static char events[32];
static size_t event_count;
static unsigned char output[64];
static size_t output_length;
static unsigned long allocation_storage[128];
static int allocation_in_use;
static size_t allocation_calls;
static size_t free_calls;

static int same_string(const char *left, const char *right)
{
    size_t i = 0;

    while (left[i] != '\0' && right[i] != '\0') {
        if (left[i] != right[i]) {
            return 0;
        }
        ++i;
    }
    return left[i] == right[i];
}

static void record_event(char event)
{
    if (event_count < sizeof(events)) {
        events[event_count++] = event;
    }
}

static void reset_scripts(void)
{
    write_count = 0;
    write_index = 0;
    open_count = 0;
    open_index = 0;
    close_count = 0;
    close_index = 0;
    event_count = 0;
    output_length = 0;
    allocation_calls = 0;
    free_calls = 0;
}

static void push_write(int fd, long result)
{
    writes[write_count].fd = fd;
    writes[write_count].result = result;
    ++write_count;
}

static void push_open(const char *path, int flags, long result)
{
    opens[open_count].path = path;
    opens[open_count].flags = flags;
    opens[open_count].result = result;
    ++open_count;
}

static void push_close(int fd, long result)
{
    closes[close_count].fd = fd;
    closes[close_count].result = result;
    ++close_count;
}

long mini_test_write(int fd, const void *buf, unsigned long count)
{
    const unsigned char *bytes = (const unsigned char *)buf;
    long result;
    size_t i;

    record_event('W');
    if (write_index >= write_count || fd != writes[write_index].fd) {
        return -EINVAL;
    }
    result = writes[write_index++].result;
    if (result > 0) {
        if ((unsigned long)result > count ||
            output_length + (size_t)result > sizeof(output)) {
            return -EINVAL;
        }
        for (i = 0; i < (size_t)result; ++i) {
            output[output_length++] = bytes[i];
        }
    }
    return result;
}

long mini_test_read(int fd, void *buf, unsigned long count)
{
    (void)fd;
    (void)buf;
    (void)count;
    return -EINVAL;
}

long mini_test_openat(int dirfd, const char *path, int flags, unsigned int mode)
{
    const struct open_step *step;

    record_event('O');
    if (open_index >= open_count) {
        return -EINVAL;
    }
    step = &opens[open_index++];
    if (dirfd != TEST_AT_FDCWD || mode != 0666U ||
        !same_string(path, step->path) || flags != step->flags) {
        return -EINVAL;
    }
    return step->result;
}

long mini_test_close(int fd)
{
    const struct close_step *step;

    record_event('C');
    if (close_index >= close_count) {
        return -EINVAL;
    }
    step = &closes[close_index++];
    if (fd != step->fd) {
        return -EINVAL;
    }
    return step->result;
}

void *mini_test_malloc(size_t size)
{
    ++allocation_calls;
    if (allocation_in_use || size > sizeof(allocation_storage)) {
        errno = ENOMEM;
        return (void *)0;
    }
    allocation_in_use = 1;
    return allocation_storage;
}

void mini_test_free(void *ptr)
{
    if (ptr == allocation_storage && allocation_in_use) {
        allocation_in_use = 0;
        ++free_calls;
    }
}

static int events_equal(const char *expected)
{
    size_t i = 0;

    while (expected[i] != '\0') {
        if (i >= event_count || events[i] != expected[i]) {
            return 0;
        }
        ++i;
    }
    return i == event_count;
}

static int output_equal(const char *expected)
{
    size_t i = 0;

    while (expected[i] != '\0') {
        if (i >= output_length || output[i] != (unsigned char)expected[i]) {
            return 0;
        }
        ++i;
    }
    return i == output_length;
}

int main(void)
{
    char caller_buffer[4];
    FILE *stream;
    FILE *identity;

    reset_scripts();
    push_open("old", TEST_O_WRONLY | TEST_O_CREAT | TEST_O_TRUNC, 10);
    stream = fopen("old", "w");
    if (stream == (FILE *)0 || !allocation_in_use || allocation_calls != 1 ||
        setvbuf(stream, caller_buffer, _IOFBF, sizeof(caller_buffer)) != 0 ||
        fputs("xy", stream) == EOF) {
        return 1;
    }
    identity = stream;
    event_count = 0;
    push_write(10, 2);
    push_close(10, 0);
    push_open("new", TEST_O_RDWR | TEST_O_CREAT | TEST_O_TRUNC, 11);
    errno = ERANGE;
    if (freopen("new", "w+", stream) != identity || errno != ERANGE ||
        !events_equal("WCO") || !output_equal("xy") || free_calls != 0 ||
        !allocation_in_use) {
        return 2;
    }
    event_count = 0;
    if (fputs("abc", stream) == EOF || caller_buffer[0] != 'a' ||
        event_count != 0) {
        return 3;
    }
    push_write(11, 3);
    if (fflush(stream) != 0 || !output_equal("xyabc") ||
        !events_equal("W")) {
        return 4;
    }
    push_close(11, 0);
    if (fclose(stream) != 0 || allocation_in_use || free_calls != 1) {
        return 5;
    }

    reset_scripts();
    push_open("stable", TEST_O_RDONLY, 20);
    stream = fopen("stable", "r");
    if (stream == (FILE *)0) {
        return 6;
    }
    event_count = 0;
    errno = ERANGE;
    if (freopen("ignored", "r++", stream) != (FILE *)0 || errno != EINVAL ||
        event_count != 0) {
        return 7;
    }
    push_close(20, 0);
    if (fclose(stream) != 0 || free_calls != 1 || allocation_in_use) {
        return 8;
    }

    reset_scripts();
    push_open("old-missing", TEST_O_RDONLY, 21);
    stream = fopen("old-missing", "r");
    if (stream == (FILE *)0) {
        return 9;
    }
    event_count = 0;
    push_close(21, 0);
    push_open("missing", TEST_O_RDONLY, -ENOENT);
    errno = ERANGE;
    if (freopen("missing", "r", stream) != (FILE *)0 || errno != ENOENT ||
        !events_equal("CO") || allocation_in_use || free_calls != 1) {
        return 10;
    }

    reset_scripts();
    push_open("flush-fail", TEST_O_WRONLY | TEST_O_CREAT | TEST_O_TRUNC, 22);
    stream = fopen("flush-fail", "w");
    if (stream == (FILE *)0 || fputc('Q', stream) != 'Q') {
        return 11;
    }
    event_count = 0;
    push_write(22, -EIO);
    push_close(22, 0);
    errno = ERANGE;
    if (freopen("never-opened", "w", stream) != (FILE *)0 || errno != EIO ||
        !events_equal("WC") || allocation_in_use || free_calls != 1) {
        return 12;
    }

    reset_scripts();
    push_open("close-fail", TEST_O_RDONLY, 23);
    stream = fopen("close-fail", "r");
    if (stream == (FILE *)0) {
        return 13;
    }
    event_count = 0;
    push_close(23, -EIO);
    errno = ERANGE;
    if (freopen("never-opened", "r", stream) != (FILE *)0 || errno != EIO ||
        !events_equal("C") || allocation_in_use || free_calls != 1) {
        return 14;
    }

    reset_scripts();
    if (fputs("S", stdout) == EOF) {
        return 15;
    }
    event_count = 0;
    push_write(1, 1);
    push_close(1, 0);
    push_open("stdout-file", TEST_O_WRONLY | TEST_O_CREAT | TEST_O_TRUNC, 30);
    errno = ERANGE;
    if (freopen("stdout-file", "w", stdout) != stdout || errno != ERANGE ||
        !events_equal("WCO") || free_calls != 0 || allocation_in_use) {
        return 16;
    }
    event_count = 0;
    if (fputc('R', stdout) != 'R' || event_count != 0) {
        return 17;
    }
    push_write(30, 1);
    if (fflush(stdout) != 0 || !output_equal("SR") ||
        !events_equal("W")) {
        return 18;
    }
    push_close(30, 0);
    if (fclose(stdout) != 0 || free_calls != 0 || allocation_in_use) {
        return 19;
    }

    return 0;
}
