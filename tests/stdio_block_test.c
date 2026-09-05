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

static struct read_step reads[64];
static size_t read_count;
static size_t read_index;
static struct write_step writes[64];
static size_t write_count;
static size_t write_index;
static struct seek_step seeks[64];
static size_t seek_count;
static size_t seek_index;
static unsigned long read_requested[64];
static unsigned long write_requested[64];
static size_t read_calls;
static size_t write_calls;
static size_t seek_calls;
static unsigned char output[512];
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

    if (read_index >= read_count || read_calls >= 64) {
        return -EINVAL;
    }
    step = &reads[read_index++];
    read_requested[read_calls++] = count;
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

    if (write_index >= write_count || write_calls >= 64) {
        return -EINVAL;
    }
    step = &writes[write_index++];
    write_requested[write_calls++] = count;
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

    if (seek_index >= seek_count || seek_calls >= 64) {
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
    unsigned char large[260];
    size_t result;
    size_t i;
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
        errno != ERANGE || read_calls != 3 || read_requested[0] != 6 ||
        read_requested[1] != 3 || read_requested[2] != 1) {
        return 2;
    }

    reset_scripts();
    push_read(0, 2, "AB");
    push_read(0, -EIO, "");
    errno = ERANGE;
    result = fread(buffer, 2, 2, stdin);
    if (result != 1 || !bytes_equal(buffer, "AB", 2) ||
        !ferror(stdin) || feof(stdin) || errno != EIO || read_calls != 2 ||
        read_requested[0] != 4 || read_requested[1] != 2) {
        return 3;
    }
    clearerr(stdin);

    reset_scripts();
    errno = ERANGE;
    if (fread(buffer, 1, 1, stdout) != 0 || errno != EINVAL ||
        !ferror(stdout) || read_calls != 0) {
        return 4;
    }
    clearerr(stdout);

    reset_scripts();
    errno = ERANGE;
    result = fwrite("ABCDEF", 2, 3, stdout);
    if (result != 3 || write_calls != 0 || output_length != 0 ||
        errno != ERANGE || ferror(stdout)) {
        return 5;
    }
    push_write(1, 3);
    push_write(1, 2);
    push_write(1, 1);
    if (fflush(stdout) != 0 || !bytes_equal(output, "ABCDEF", 6) ||
        output_length != 6 || write_calls != 3 ||
        write_requested[0] != 6 || write_requested[1] != 3 ||
        write_requested[2] != 1 || errno != ERANGE) {
        return 6;
    }

    for (i = 0; i < 256; ++i) {
        large[i] = 'A';
    }
    large[256] = 'W';
    large[257] = 'X';
    large[258] = 'Y';
    large[259] = 'Z';

    reset_scripts();
    if (fwrite(large, 1, 256, stdout) != 256 || write_calls != 0) {
        return 7;
    }
    push_write(1, 128);
    push_write(1, -EIO);
    errno = ERANGE;
    if (fwrite(large + 256, 1, 4, stdout) != 0 || errno != EIO ||
        !ferror(stdout) || output_length != 128 || write_calls != 2 ||
        write_requested[0] != 256 || write_requested[1] != 128) {
        return 8;
    }
    clearerr(stdout);
    push_write(1, 128);
    errno = ERANGE;
    if (fflush(stdout) != 0 || errno != ERANGE || ferror(stdout) ||
        output_length != 256) {
        return 9;
    }
    if (fwrite(large + 256, 1, 4, stdout) != 4 || write_calls != 3) {
        return 10;
    }
    push_write(1, 4);
    if (fflush(stdout) != 0 || output_length != 260) {
        return 11;
    }
    for (i = 0; i < 256; ++i) {
        if (output[i] != 'A') {
            return 12;
        }
    }
    if (!bytes_equal(output + 256, "WXYZ", 4)) {
        return 13;
    }

    reset_scripts();
    errno = ERANGE;
    if (fwrite(buffer, (size_t)-1, 2, stdout) != 0 || errno != EINVAL ||
        !ferror(stdout) || write_calls != 0) {
        return 14;
    }
    clearerr(stdout);

    reset_scripts();
    if (fwrite("AB", 1, 2, stdout) != 2 || write_calls != 0) {
        return 15;
    }
    push_write(1, 2);
    push_seek(1, 5L, SEEK_SET, 5L);
    errno = ERANGE;
    if (fseek(stdout, 5L, SEEK_SET) != 0 || errno != ERANGE ||
        output_length != 2 || !bytes_equal(output, "AB", 2) ||
        seek_calls != 1) {
        return 16;
    }
    push_seek(1, 0L, SEEK_CUR, 5L);
    if (ftell(stdout) != 5L || seek_calls != 2) {
        return 17;
    }
    if (fwrite("C", 1, 1, stdout) != 1) {
        return 18;
    }
    push_seek(1, 0L, SEEK_CUR, 5L);
    position = ftell(stdout);
    if (position != 6L || seek_calls != 3 || output_length != 2) {
        return 19;
    }
    push_write(1, 1);
    push_seek(1, 0L, SEEK_SET, 0L);
    rewind(stdout);
    if (ferror(stdout) || feof(stdout) || output_length != 3 ||
        !bytes_equal(output, "ABC", 3) || seek_calls != 4) {
        return 20;
    }

    reset_scripts();
    if (fwrite("D", 1, 1, stdout) != 1) {
        return 21;
    }
    push_write(1, -EIO);
    errno = ERANGE;
    if (fseek(stdout, 7L, SEEK_SET) == 0 || errno != EIO ||
        !ferror(stdout) || seek_calls != 0 || output_length != 0) {
        return 22;
    }
    clearerr(stdout);
    push_write(1, 1);
    push_seek(1, 7L, SEEK_SET, 7L);
    errno = ERANGE;
    if (fseek(stdout, 7L, SEEK_SET) != 0 || errno != ERANGE ||
        ferror(stdout) || seek_calls != 1 || !bytes_equal(output, "D", 1)) {
        return 23;
    }

    reset_scripts();
    if (fwrite("E", 1, 1, stdout) != 1) {
        return 24;
    }
    push_write(1, 1);
    push_seek(1, 0L, SEEK_SET, -EIO);
    errno = ERANGE;
    if (fseek(stdout, 0L, SEEK_SET) == 0 || errno != EIO ||
        seek_calls != 1 || !bytes_equal(output, "E", 1)) {
        return 25;
    }
    errno = ERANGE;
    if (fflush(stdout) != 0 || errno != ERANGE) {
        return 26;
    }

    reset_scripts();
    push_read(0, 0, "");
    if (fread(buffer, 1, 1, stdin) != 0 || !feof(stdin)) {
        return 27;
    }
    push_seek(0, 0L, SEEK_SET, 0L);
    errno = ERANGE;
    if (fseek(stdin, 0L, SEEK_SET) != 0 || feof(stdin) || errno != ERANGE) {
        return 28;
    }
    push_read(0, 1, "Q");
    if (fread(buffer, 1, 1, stdin) != 1 || buffer[0] != 'Q') {
        return 29;
    }

    reset_scripts();
    push_read(0, 0, "");
    if (fread(buffer, 1, 1, stdin) != 0 || !feof(stdin)) {
        return 30;
    }
    push_seek(0, 0L, SEEK_SET, -EIO);
    errno = ERANGE;
    if (fseek(stdin, 0L, SEEK_SET) == 0 || errno != EIO || !feof(stdin) ||
        seek_calls != 1) {
        return 31;
    }

    reset_scripts();
    errno = ERANGE;
    if (fseek(stdin, 0L, 99) == 0 || errno != EINVAL || seek_calls != 0) {
        return 32;
    }
    push_seek(0, 0L, SEEK_CUR, 17L);
    errno = ERANGE;
    if (ftell(stdin) != 17L || errno != ERANGE || seek_calls != 1) {
        return 33;
    }
    push_seek(0, 0L, SEEK_CUR, -EIO);
    if (ftell(stdin) != -1L || errno != EIO || seek_calls != 2 || ferror(stdin)) {
        return 34;
    }

    reset_scripts();
    push_read(0, 0, "");
    if (fread(buffer, 1, 1, stdin) != 0 || !feof(stdin)) {
        return 35;
    }
    errno = ERANGE;
    if (fwrite("X", 1, 1, stdin) != 0 || errno != EINVAL || !ferror(stdin)) {
        return 36;
    }
    push_seek(0, 0L, SEEK_SET, -EIO);
    errno = ERANGE;
    rewind(stdin);
    if (feof(stdin) || ferror(stdin) || errno != EIO || seek_calls != 1) {
        return 37;
    }

    reset_scripts();
    errno = ERANGE;
    if (ftell((FILE *)0) != -1L || errno != EINVAL || seek_calls != 0) {
        return 38;
    }
    errno = ERANGE;
    if (fseek((FILE *)0, 0L, SEEK_SET) == 0 || errno != EINVAL ||
        seek_calls != 0) {
        return 39;
    }

    return 0;
}
