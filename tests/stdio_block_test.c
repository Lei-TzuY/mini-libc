#include <errno.h>
#include <stddef.h>
#include <stdio.h>

struct read_step {
    int fd;
    long result;
    const char *data;
};

struct write_step {
    int fd;
    long result;
};

struct seek_step {
    int fd;
    long offset;
    int whence;
    long result;
};

static struct read_step reads[24];
static size_t read_count;
static size_t read_index;
static struct write_step writes[24];
static size_t write_count;
static size_t write_index;
static struct seek_step seeks[24];
static size_t seek_count;
static size_t seek_index;
static unsigned long requested[24];
static size_t read_calls;
static size_t write_calls;
static size_t seek_calls;
static unsigned char output[64];
static size_t output_length;

static void reset_scripts(void)
{
    read_count = 0;
    read_index = 0;
    write_count = 0;
    write_index = 0;
    seek_count = 0;
    seek_index = 0;
    read_calls = 0;
    write_calls = 0;
    seek_calls = 0;
    output_length = 0;
    clearerr(stdin);
    clearerr(stdout);
}

static void push_read(int fd, long result, const char *data)
{
    reads[read_count].fd = fd;
    reads[read_count].result = result;
    reads[read_count].data = data;
    ++read_count;
}

static void push_write(int fd, long result)
{
    writes[write_count].fd = fd;
    writes[write_count].result = result;
    ++write_count;
}

static void push_seek(int fd, long offset, int whence, long result)
{
    seeks[seek_count].fd = fd;
    seeks[seek_count].offset = offset;
    seeks[seek_count].whence = whence;
    seeks[seek_count].result = result;
    ++seek_count;
}

long mini_test_read(int fd, void *buf, unsigned long count)
{
    const struct read_step *step;
    unsigned char *bytes = (unsigned char *)buf;
    size_t i;

    if (read_index >= read_count || read_calls >= 24) {
        return -EINVAL;
    }
    step = &reads[read_index++];
    requested[read_calls++] = count;
    if (fd != step->fd || step->result > (long)count) {
        return -EINVAL;
    }
    if (step->result > 0) {
        for (i = 0; i < (size_t)step->result; ++i) {
            bytes[i] = (unsigned char)step->data[i];
        }
    }
    return step->result;
}

long mini_test_write(int fd, const void *buf, unsigned long count)
{
    const struct write_step *step;
    const unsigned char *bytes = (const unsigned char *)buf;
    size_t i;

    if (write_index >= write_count || write_calls >= 24) {
        return -EINVAL;
    }
    step = &writes[write_index++];
    requested[write_calls++] = count;
    if (fd != step->fd || step->result > (long)count) {
        return -EINVAL;
    }
    if (step->result > 0) {
        if (output_length + (size_t)step->result > sizeof(output)) {
            return -EINVAL;
        }
        for (i = 0; i < (size_t)step->result; ++i) {
            output[output_length++] = bytes[i];
        }
    }
    return step->result;
}

long mini_test_lseek(int fd, long offset, int whence)
{
    const struct seek_step *step;

    if (seek_index >= seek_count || seek_calls >= 24) {
        return -EINVAL;
    }
    step = &seeks[seek_index++];
    ++seek_calls;
    if (fd != step->fd || offset != step->offset || whence != step->whence) {
        return -EINVAL;
    }
    return step->result;
}

static int bytes_equal(const unsigned char *actual, const char *expected,
                       size_t length)
{
    size_t i;

    for (i = 0; i < length; ++i) {
        if (actual[i] != (unsigned char)expected[i]) {
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    unsigned char buffer[8];
    size_t result;
    long position;

    reset_scripts();
    errno = ERANGE;
    if (fread(buffer, 0, 9, stdin) != 0 ||
        fwrite(buffer, 9, 0, stdout) != 0 || errno != ERANGE ||
        read_calls != 0 || write_calls != 0) {
        return 1;
    }

    reset_scripts();
    buffer[5] = 0x7fU;
    push_read(0, 3, "ABC");
    push_read(0, 2, "DE");
    push_read(0, 0, "");
    errno = ERANGE;
    result = fread(buffer, 2, 3, stdin);
    if (result != 2 || !bytes_equal(buffer, "ABCDE", 5) ||
        buffer[5] != 0x7fU || !feof(stdin) || ferror(stdin) ||
        errno != ERANGE || read_calls != 3 || requested[0] != 6 ||
        requested[1] != 3 || requested[2] != 1) {
        return 2;
    }

    reset_scripts();
    push_read(0, 2, "AB");
    push_read(0, -EIO, "");
    errno = ERANGE;
    result = fread(buffer, 2, 2, stdin);
    if (result != 1 || !bytes_equal(buffer, "AB", 2) ||
        !ferror(stdin) || feof(stdin) || errno != EIO || read_calls != 2 ||
        requested[0] != 4 || requested[1] != 2) {
        return 3;
    }

    reset_scripts();
    errno = ERANGE;
    if (fread(buffer, 1, 1, stdout) != 0 || errno != EINVAL ||
        !ferror(stdout) || read_calls != 0) {
        return 4;
    }

    reset_scripts();
    push_write(1, 3);
    push_write(1, 2);
    push_write(1, 1);
    errno = ERANGE;
    result = fwrite("ABCDEF", 2, 3, stdout);
    if (result != 3 || output_length != 6 ||
        !bytes_equal(output, "ABCDEF", 6) || ferror(stdout) ||
        errno != ERANGE || write_calls != 3 || requested[0] != 6 ||
        requested[1] != 3 || requested[2] != 1) {
        return 5;
    }

    reset_scripts();
    push_write(1, 3);
    push_write(1, -EIO);
    errno = ERANGE;
    result = fwrite("ABCDEF", 2, 3, stdout);
    if (result != 1 || output_length != 3 ||
        !bytes_equal(output, "ABC", 3) || !ferror(stdout) ||
        errno != EIO || write_calls != 2) {
        return 6;
    }

    reset_scripts();
    push_write(1, 2);
    push_write(1, 0);
    errno = ERANGE;
    result = fwrite("ABCD", 2, 2, stdout);
    if (result != 1 || output_length != 2 || !ferror(stdout) ||
        errno != EIO || write_calls != 2) {
        return 7;
    }

    reset_scripts();
    errno = ERANGE;
    if (fread(buffer, (size_t)-1, 2, stdin) != 0 || errno != EINVAL ||
        !ferror(stdin) || read_calls != 0) {
        return 8;
    }
    clearerr(stdin);
    errno = ERANGE;
    if (fwrite(buffer, (size_t)-1, 2, stdout) != 0 || errno != EINVAL ||
        !ferror(stdout) || write_calls != 0) {
        return 9;
    }

    reset_scripts();
    push_read(0, 0, "");
    if (fread(buffer, 1, 1, stdin) != 0 || !feof(stdin)) {
        return 10;
    }
    errno = ERANGE;
    if (fwrite("X", 1, 1, stdin) != 0 || errno != EINVAL || !ferror(stdin)) {
        return 11;
    }
    push_seek(0, 5L, SEEK_SET, 5L);
    errno = ERANGE;
    if (fseek(stdin, 5L, SEEK_SET) != 0 || feof(stdin) || !ferror(stdin) ||
        errno != ERANGE || seek_calls != 1) {
        return 12;
    }

    reset_scripts();
    push_read(0, 0, "");
    if (fread(buffer, 1, 1, stdin) != 0 || !feof(stdin)) {
        return 13;
    }
    push_seek(0, 0L, SEEK_SET, -EIO);
    errno = ERANGE;
    if (fseek(stdin, 0L, SEEK_SET) == 0 || errno != EIO || !feof(stdin) ||
        ferror(stdin) || seek_calls != 1) {
        return 14;
    }

    reset_scripts();
    errno = ERANGE;
    if (fseek(stdin, 0L, 99) == 0 || errno != EINVAL || seek_calls != 0 ||
        ferror(stdin) || feof(stdin)) {
        return 15;
    }

    reset_scripts();
    push_seek(0, 0L, SEEK_CUR, 17L);
    errno = ERANGE;
    position = ftell(stdin);
    if (position != 17L || errno != ERANGE || seek_calls != 1) {
        return 16;
    }
    push_seek(0, 0L, SEEK_CUR, -EIO);
    errno = ERANGE;
    position = ftell(stdin);
    if (position != -1L || errno != EIO || seek_calls != 2 || ferror(stdin)) {
        return 17;
    }

    reset_scripts();
    push_read(0, 0, "");
    if (fread(buffer, 1, 1, stdin) != 0 || !feof(stdin)) {
        return 18;
    }
    errno = ERANGE;
    if (fwrite("X", 1, 1, stdin) != 0 || !ferror(stdin)) {
        return 19;
    }
    push_seek(0, 0L, SEEK_SET, 0L);
    errno = ERANGE;
    rewind(stdin);
    if (feof(stdin) || ferror(stdin) || errno != ERANGE || seek_calls != 1) {
        return 20;
    }

    reset_scripts();
    push_read(0, 0, "");
    if (fread(buffer, 1, 1, stdin) != 0 || !feof(stdin)) {
        return 21;
    }
    errno = ERANGE;
    if (fwrite("X", 1, 1, stdin) != 0 || !ferror(stdin)) {
        return 22;
    }
    push_seek(0, 0L, SEEK_SET, -EIO);
    errno = ERANGE;
    rewind(stdin);
    if (feof(stdin) || ferror(stdin) || errno != EIO || seek_calls != 1) {
        return 23;
    }

    reset_scripts();
    errno = ERANGE;
    if (ftell((FILE *)0) != -1L || errno != EINVAL || seek_calls != 0) {
        return 24;
    }
    errno = ERANGE;
    if (fseek((FILE *)0, 0L, SEEK_SET) == 0 || errno != EINVAL ||
        seek_calls != 0) {
        return 25;
    }

    return 0;
}
