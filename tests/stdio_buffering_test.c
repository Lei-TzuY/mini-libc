#include <errno.h>
#include <stddef.h>
#include <stdio.h>

static unsigned char output[128];
static size_t output_length;
static size_t write_calls;
static unsigned long last_write_count;

static const char *read_data;
static long read_result;
static size_t read_calls;
static unsigned long last_read_count;

static long expected_seek_offset;
static long seek_result;
static size_t seek_calls;

static unsigned char allocation_storage[128];
static int allocation_in_use;
static size_t allocation_calls;
static size_t free_calls;

static void reset_io(void)
{
    output_length = 0;
    write_calls = 0;
    last_write_count = 0;
    read_data = (const char *)0;
    read_result = -EINVAL;
    read_calls = 0;
    last_read_count = 0;
    expected_seek_offset = 0;
    seek_result = 0;
    seek_calls = 0;
    clearerr(stdin);
    clearerr(stdout);
}

long mini_test_write(int fd, const void *buf, unsigned long count)
{
    const unsigned char *bytes = (const unsigned char *)buf;
    size_t i;

    if (fd != 1 || output_length + (size_t)count > sizeof(output)) {
        return -EINVAL;
    }
    ++write_calls;
    last_write_count = count;
    for (i = 0; i < (size_t)count; ++i) {
        output[output_length++] = bytes[i];
    }
    return (long)count;
}

long mini_test_read(int fd, void *buf, unsigned long count)
{
    unsigned char *bytes = (unsigned char *)buf;
    size_t i;

    if (fd != 0 || read_data == (const char *)0 || read_result < 0 ||
        (unsigned long)read_result > count) {
        return -EINVAL;
    }
    ++read_calls;
    last_read_count = count;
    for (i = 0; i < (size_t)read_result; ++i) {
        bytes[i] = (unsigned char)read_data[i];
    }
    return read_result;
}

long mini_sys_lseek(int fd, long offset, int whence)
{
    ++seek_calls;
    if (fd != 0 || whence != SEEK_CUR || offset != expected_seek_offset) {
        return -EINVAL;
    }
    return seek_result;
}

long mini_test_openat(int dirfd, const char *path, int flags, unsigned int mode)
{
    (void)dirfd;
    (void)path;
    (void)flags;
    (void)mode;
    return -EINVAL;
}

long mini_test_close(int fd)
{
    (void)fd;
    return -EINVAL;
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

static int output_is(const char *text, size_t length)
{
    size_t i;

    if (output_length != length) {
        return 0;
    }
    for (i = 0; i < length; ++i) {
        if (output[i] != (unsigned char)text[i]) {
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    char full[4];
    char line[8];
    char readbuf[4];
    char smallread[2];
    char setbuf_storage[BUFSIZ];

    reset_io();
    errno = ERANGE;
    if (setvbuf(stdout, full, _IOFBF, sizeof(full)) != 0 || errno != ERANGE ||
        fputs("abc", stdout) == EOF || write_calls != 0 ||
        full[0] != 'a' || full[1] != 'b' || full[2] != 'c' ||
        fputc('d', stdout) != 'd' || write_calls != 1 ||
        last_write_count != 4 || !output_is("abcd", 4)) {
        return 1;
    }

    reset_io();
    if (setvbuf(stdout, line, _IOLBF, sizeof(line)) != 0 ||
        fputs("xy", stdout) == EOF || write_calls != 0 ||
        line[0] != 'x' || line[1] != 'y' ||
        fputc('\n', stdout) != '\n' || write_calls != 1 ||
        !output_is("xy\n", 3)) {
        return 2;
    }

    reset_io();
    if (setvbuf(stdout, (char *)0, _IONBF, 0U) != 0 ||
        fputs("uv", stdout) == EOF || write_calls != 1 ||
        last_write_count != 2 || !output_is("uv", 2)) {
        return 3;
    }

    reset_io();
    allocation_calls = 0;
    free_calls = 0;
    allocation_in_use = 0;
    if (setvbuf(stdout, (char *)0, _IOFBF, 7U) != 0 ||
        allocation_calls != 1 || !allocation_in_use ||
        setvbuf(stdout, full, _IOFBF, sizeof(full)) != 0 ||
        free_calls != 1 || allocation_in_use) {
        return 4;
    }
    setbuf(stdout, setbuf_storage);
    if (fputs("q", stdout) == EOF || write_calls != 0 ||
        setbuf_storage[0] != 'q') {
        return 5;
    }
    setbuf(stdout, (char *)0);
    if (write_calls != 1 || !output_is("q", 1) || free_calls != 1) {
        return 6;
    }

    reset_io();
    if (setvbuf(stdin, readbuf, _IOFBF, sizeof(readbuf)) != 0) {
        return 7;
    }
    read_data = "abcd";
    read_result = 4;
    if (fgetc(stdin) != 'a' || read_calls != 1 || last_read_count != 4 ||
        readbuf[0] != 'a' || readbuf[3] != 'd') {
        return 8;
    }
    expected_seek_offset = -3L;
    seek_result = 1L;
    if (setvbuf(stdin, smallread, _IOFBF, sizeof(smallread)) != 0 ||
        seek_calls != 1) {
        return 9;
    }
    read_data = "XY";
    read_result = 2;
    if (fgetc(stdin) != 'X' || read_calls != 2 || last_read_count != 2) {
        return 10;
    }

    expected_seek_offset = -1L;
    seek_result = 0L;
    if (setvbuf(stdin, readbuf, _IOFBF, sizeof(readbuf)) != 0 ||
        seek_calls != 2) {
        return 11;
    }
    read_data = "wxyz";
    read_result = 4;
    if (fgetc(stdin) != 'w' || read_calls != 3 || last_read_count != 4) {
        return 12;
    }
    expected_seek_offset = -3L;
    seek_result = -EIO;
    errno = ERANGE;
    if (setvbuf(stdin, smallread, _IOFBF, sizeof(smallread)) != EOF ||
        errno != EIO || seek_calls != 3 || fgetc(stdin) != 'x' ||
        read_calls != 3) {
        return 13;
    }

    errno = ERANGE;
    if (setvbuf(stdout, full, 99, sizeof(full)) != EOF || errno != EINVAL ||
        setvbuf(stdout, full, _IOFBF, 0U) != EOF || errno != EINVAL) {
        return 14;
    }

    return 0;
}
