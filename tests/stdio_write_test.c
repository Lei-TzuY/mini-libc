#include <errno.h>
#include <stddef.h>
#include <stdio.h>

#define TEST_AT_FDCWD (-100)
#define TEST_O_RDONLY 0
#define TEST_O_WRONLY 1
#define TEST_O_RDWR 2
#define TEST_O_CREAT 64
#define TEST_O_TRUNC 512
#define TEST_O_APPEND 1024

struct scripted_write {
    int fd;
    long result;
};

struct scripted_read {
    int fd;
    long result;
    unsigned char byte;
};

struct scripted_open {
    const char *path;
    int flags;
    unsigned int mode;
    long result;
};

struct scripted_close {
    int fd;
    long result;
};

static struct scripted_write write_script[64];
static size_t write_script_count;
static size_t write_script_index;
static struct scripted_read read_script[64];
static size_t read_script_count;
static size_t read_script_index;
static struct scripted_open open_script[64];
static size_t open_script_count;
static size_t open_script_index;
static struct scripted_close close_script[64];
static size_t close_script_count;
static size_t close_script_index;
static unsigned char output[256];
static size_t output_length;
static unsigned long counts[64];
static int write_fds[64];
static size_t write_call_count;
static size_t read_call_count;
static size_t open_call_count;
static size_t close_call_count;

static unsigned long allocation_storage[64];
static int allocation_in_use;
static int allocation_fail;
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

static void reset_scripts(void)
{
    write_script_count = 0;
    write_script_index = 0;
    read_script_count = 0;
    read_script_index = 0;
    open_script_count = 0;
    open_script_index = 0;
    close_script_count = 0;
    close_script_index = 0;
    output_length = 0;
    write_call_count = 0;
    read_call_count = 0;
    open_call_count = 0;
    close_call_count = 0;
    allocation_fail = 0;
    allocation_calls = 0;
    free_calls = 0;
    clearerr(stdin);
    clearerr(stdout);
    clearerr(stderr);
}

static void push_write(int fd, long result)
{
    write_script[write_script_count].fd = fd;
    write_script[write_script_count].result = result;
    ++write_script_count;
}

static void push_read(int fd, long result, unsigned char byte)
{
    read_script[read_script_count].fd = fd;
    read_script[read_script_count].result = result;
    read_script[read_script_count].byte = byte;
    ++read_script_count;
}

static void push_open(const char *path, int flags, long result)
{
    open_script[open_script_count].path = path;
    open_script[open_script_count].flags = flags;
    open_script[open_script_count].mode = 0666U;
    open_script[open_script_count].result = result;
    ++open_script_count;
}

static void push_close(int fd, long result)
{
    close_script[close_script_count].fd = fd;
    close_script[close_script_count].result = result;
    ++close_script_count;
}

long mini_test_write(int fd, const void *buf, unsigned long count)
{
    const unsigned char *bytes = (const unsigned char *)buf;
    long result;
    size_t i;

    if (write_script_index >= write_script_count || write_call_count >= 64 ||
        fd != write_script[write_script_index].fd) {
        return -EINVAL;
    }

    counts[write_call_count] = count;
    write_fds[write_call_count] = fd;
    ++write_call_count;
    result = write_script[write_script_index++].result;
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
    long result;

    if (read_script_index >= read_script_count || read_call_count >= 64 ||
        count != 1 || fd != read_script[read_script_index].fd) {
        return -EINVAL;
    }

    ++read_call_count;
    result = read_script[read_script_index].result;
    if (result == 1) {
        *(unsigned char *)buf = read_script[read_script_index].byte;
    } else if (result > 1) {
        return -EINVAL;
    }
    ++read_script_index;
    return result;
}

long mini_test_openat(int dirfd, const char *path, int flags, unsigned int mode)
{
    const struct scripted_open *step;

    if (open_script_index >= open_script_count || open_call_count >= 64) {
        return -EINVAL;
    }

    step = &open_script[open_script_index++];
    ++open_call_count;
    if (dirfd != TEST_AT_FDCWD || !same_string(path, step->path) ||
        flags != step->flags || mode != step->mode) {
        return -EINVAL;
    }
    return step->result;
}

long mini_test_close(int fd)
{
    const struct scripted_close *step;

    if (close_script_index >= close_script_count || close_call_count >= 64) {
        return -EINVAL;
    }

    step = &close_script[close_script_index++];
    ++close_call_count;
    if (fd != step->fd) {
        return -EINVAL;
    }
    return step->result;
}

void *mini_test_malloc(size_t size)
{
    ++allocation_calls;
    if (allocation_fail || allocation_in_use || size > sizeof(allocation_storage)) {
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

static int bytes_equal(const char *expected, size_t length)
{
    size_t i;

    if (output_length != length) {
        return 0;
    }
    for (i = 0; i < length; ++i) {
        if (output[i] != (unsigned char)expected[i]) {
            return 0;
        }
    }
    return 1;
}

static int check_open_close(const char *path, const char *mode, int flags, int fd)
{
    FILE *stream;

    reset_scripts();
    push_open(path, flags, fd);
    push_close(fd, 0);
    errno = ERANGE;
    stream = fopen(path, mode);
    if (stream == (FILE *)0 || errno != ERANGE || allocation_calls != 1 ||
        !allocation_in_use || open_call_count != 1) {
        return 0;
    }
    if (fclose(stream) != 0 || errno != ERANGE || close_call_count != 1 ||
        free_calls != 1 || allocation_in_use) {
        return 0;
    }
    return 1;
}

int main(void)
{
    FILE *stream;
    int result;

    reset_scripts();
    push_read(0, 1, 'K');
    errno = ERANGE;
    result = fgetc(stdin);
    if (result != 'K' || errno != ERANGE || feof(stdin) || ferror(stdin) ||
        read_call_count != 1) {
        return 1;
    }

    reset_scripts();
    push_read(0, 0, 0);
    errno = ERANGE;
    result = getc(stdin);
    if (result != EOF || errno != ERANGE || !feof(stdin) || ferror(stdin) ||
        read_call_count != 1) {
        return 2;
    }
    result = fgetc(stdin);
    if (result != EOF || errno != ERANGE || read_call_count != 1) {
        return 3;
    }
    clearerr(stdin);

    reset_scripts();
    push_read(0, -EINVAL, 0);
    errno = ERANGE;
    result = getchar();
    if (result != EOF || errno != EINVAL || feof(stdin) || !ferror(stdin) ||
        read_call_count != 1) {
        return 4;
    }
    clearerr(stdin);

    reset_scripts();
    errno = ERANGE;
    if (fgetc(stdout) != EOF || errno != EINVAL || !ferror(stdout) ||
        read_call_count != 0) {
        return 5;
    }
    clearerr(stdout);

    reset_scripts();
    errno = ERANGE;
    result = puts("abcd");
    if (result < 0 || errno != ERANGE || ferror(stdout) ||
        write_call_count != 0 || output_length != 0) {
        return 6;
    }
    push_write(1, 2);
    push_write(1, 2);
    push_write(1, 1);
    if (fflush(stdout) != 0 || errno != ERANGE || !bytes_equal("abcd\n", 5) ||
        write_call_count != 3 || counts[0] != 5 || counts[1] != 3 ||
        counts[2] != 1 || write_fds[0] != 1 || write_fds[1] != 1 ||
        write_fds[2] != 1) {
        return 7;
    }

    reset_scripts();
    errno = ERANGE;
    if (putchar('Z') != 'Z' || write_call_count != 0 || output_length != 0) {
        return 8;
    }
    push_write(1, 1);
    if (fflush(stdout) != 0 || errno != ERANGE || !bytes_equal("Z", 1)) {
        return 9;
    }

    reset_scripts();
    push_write(2, 2);
    push_write(2, 1);
    errno = ERANGE;
    result = fputs("xyz", stderr);
    if (result < 0 || errno != ERANGE || ferror(stderr) ||
        !bytes_equal("xyz", 3) || write_call_count != 2 ||
        counts[0] != 3 || counts[1] != 1) {
        return 10;
    }
    if (fflush(stderr) != 0 || errno != ERANGE || write_call_count != 2) {
        return 11;
    }

    reset_scripts();
    errno = ERANGE;
    if (fputs("abcd", stdout) == EOF || write_call_count != 0) {
        return 12;
    }
    push_write(1, 2);
    push_write(1, -EINVAL);
    if (fflush(stdout) != EOF || errno != EINVAL || !ferror(stdout) ||
        !bytes_equal("ab", 2) || write_call_count != 2 ||
        counts[0] != 4 || counts[1] != 2) {
        return 13;
    }
    clearerr(stdout);
    push_write(1, 2);
    errno = ERANGE;
    if (fflush(stdout) != 0 || errno != ERANGE || ferror(stdout) ||
        !bytes_equal("abcd", 4) || write_call_count != 3 || counts[2] != 2) {
        return 14;
    }

    reset_scripts();
    errno = ERANGE;
    if (fputc('R', stdout) != 'R' || write_call_count != 0) {
        return 15;
    }
    push_write(1, 0);
    if (fflush(stdout) != EOF || errno != EIO || !ferror(stdout) ||
        write_call_count != 1 || output_length != 0) {
        return 16;
    }
    clearerr(stdout);
    push_write(1, 1);
    errno = ERANGE;
    if (fflush(stdout) != 0 || errno != ERANGE || !bytes_equal("R", 1)) {
        return 17;
    }

    reset_scripts();
    errno = ERANGE;
    if (fputc('S', stdin) != EOF || errno != EINVAL || !ferror(stdin) ||
        write_call_count != 0) {
        return 18;
    }
    clearerr(stdin);

    reset_scripts();
    errno = ERANGE;
    if (fputs("", stdout) == EOF || errno != ERANGE || ferror(stdout) ||
        write_call_count != 0 || output_length != 0) {
        return 19;
    }

    if (!check_open_close("r-file", "r", TEST_O_RDONLY, 10) ||
        !check_open_close("rp-file", "rb+", TEST_O_RDWR, 11) ||
        !check_open_close("w-file", "w", TEST_O_WRONLY | TEST_O_CREAT | TEST_O_TRUNC, 12) ||
        !check_open_close("wp-file", "w+b", TEST_O_RDWR | TEST_O_CREAT | TEST_O_TRUNC, 13) ||
        !check_open_close("a-file", "a", TEST_O_WRONLY | TEST_O_CREAT | TEST_O_APPEND, 14) ||
        !check_open_close("ap-file", "ab+", TEST_O_RDWR | TEST_O_CREAT | TEST_O_APPEND, 15)) {
        return 20;
    }

    reset_scripts();
    errno = ERANGE;
    if (fopen("bad", "r++") != (FILE *)0 || errno != EINVAL ||
        allocation_calls != 0 || open_call_count != 0) {
        return 21;
    }
    errno = ERANGE;
    if (fopen("bad", "rx") != (FILE *)0 || errno != EINVAL ||
        allocation_calls != 0 || open_call_count != 0) {
        return 22;
    }
    errno = ERANGE;
    if (fopen((const char *)0, "r") != (FILE *)0 || errno != EINVAL ||
        allocation_calls != 0 || open_call_count != 0) {
        return 23;
    }

    reset_scripts();
    allocation_fail = 1;
    errno = ERANGE;
    if (fopen("oom", "r") != (FILE *)0 || errno != ENOMEM ||
        allocation_calls != 1 || allocation_in_use || open_call_count != 0) {
        return 24;
    }

    reset_scripts();
    push_open("missing", TEST_O_RDONLY, -ENOENT);
    errno = ERANGE;
    if (fopen("missing", "r") != (FILE *)0 || errno != ENOENT ||
        allocation_calls != 1 || free_calls != 1 || allocation_in_use ||
        open_call_count != 1 || close_call_count != 0) {
        return 25;
    }

    reset_scripts();
    push_open("rw", TEST_O_RDWR | TEST_O_CREAT | TEST_O_TRUNC, 20);
    errno = ERANGE;
    stream = fopen("rw", "w+");
    if (stream == (FILE *)0 || fputs("hi", stream) == EOF ||
        write_call_count != 0) {
        return 26;
    }
    if (fgetc(stream) != EOF || errno != EINVAL || !ferror(stream) ||
        read_call_count != 0) {
        return 27;
    }
    clearerr(stream);
    push_write(20, 2);
    errno = ERANGE;
    if (fflush(stream) != 0 || errno != ERANGE || !bytes_equal("hi", 2)) {
        return 28;
    }
    push_read(20, 1, 'K');
    if (fgetc(stream) != 'K' || errno != ERANGE || ferror(stream)) {
        return 29;
    }
    push_close(20, 0);
    if (fclose(stream) != 0 || free_calls != 1 || allocation_in_use) {
        return 30;
    }

    reset_scripts();
    push_open("eof-update", TEST_O_RDWR, 21);
    stream = fopen("eof-update", "r+");
    if (stream == (FILE *)0) {
        return 31;
    }
    push_read(21, 0, 0);
    errno = ERANGE;
    if (fgetc(stream) != EOF || !feof(stream) || ferror(stream)) {
        return 32;
    }
    if (fputc('Z', stream) != 'Z' || errno != ERANGE || write_call_count != 0) {
        return 33;
    }
    push_write(21, 1);
    push_close(21, 0);
    if (fclose(stream) != 0 || !bytes_equal("Z", 1) || free_calls != 1) {
        return 34;
    }

    reset_scripts();
    push_open("close-flush", TEST_O_WRONLY | TEST_O_CREAT | TEST_O_TRUNC, 22);
    stream = fopen("close-flush", "w");
    if (stream == (FILE *)0 || fputs("xy", stream) == EOF) {
        return 35;
    }
    push_write(22, 1);
    push_write(22, 1);
    push_close(22, 0);
    errno = ERANGE;
    if (fclose(stream) != 0 || errno != ERANGE || !bytes_equal("xy", 2) ||
        close_call_count != 1 || free_calls != 1 || allocation_in_use) {
        return 36;
    }

    reset_scripts();
    push_open("flush-fail", TEST_O_WRONLY | TEST_O_CREAT | TEST_O_TRUNC, 23);
    stream = fopen("flush-fail", "w");
    if (stream == (FILE *)0 || fputs("xy", stream) == EOF) {
        return 37;
    }
    push_write(23, 1);
    push_write(23, -EIO);
    push_close(23, 0);
    errno = ERANGE;
    if (fclose(stream) != EOF || errno != EIO || !bytes_equal("x", 1) ||
        close_call_count != 1 || free_calls != 1 || allocation_in_use) {
        return 38;
    }

    reset_scripts();
    push_open("close-fail", TEST_O_WRONLY | TEST_O_CREAT | TEST_O_TRUNC, 24);
    stream = fopen("close-fail", "w");
    if (stream == (FILE *)0 || fputc('Q', stream) != 'Q') {
        return 39;
    }
    push_write(24, 1);
    push_close(24, -EIO);
    errno = ERANGE;
    if (fclose(stream) != EOF || errno != EIO || !bytes_equal("Q", 1) ||
        close_call_count != 1 || free_calls != 1 || allocation_in_use) {
        return 40;
    }

    reset_scripts();
    push_open("flush-all", TEST_O_WRONLY | TEST_O_CREAT | TEST_O_TRUNC, 25);
    stream = fopen("flush-all", "w");
    if (stream == (FILE *)0 || fputc('F', stream) != 'F' ||
        fputc('S', stdout) != 'S' || write_call_count != 0) {
        return 41;
    }
    push_write(25, 1);
    push_write(1, 1);
    errno = ERANGE;
    if (fflush((FILE *)0) != 0 || errno != ERANGE || !bytes_equal("FS", 2) ||
        write_call_count != 2 || write_fds[0] != 25 || write_fds[1] != 1) {
        return 42;
    }
    push_close(25, 0);
    if (fclose(stream) != 0 || free_calls != 1 || allocation_in_use) {
        return 43;
    }

    reset_scripts();
    errno = ERANGE;
    if (fflush(stdin) != 0 || errno != ERANGE || write_call_count != 0) {
        return 44;
    }
    if (fclose((FILE *)0) != EOF || errno != EINVAL || close_call_count != 0 ||
        free_calls != 0) {
        return 45;
    }

    return 0;
}
