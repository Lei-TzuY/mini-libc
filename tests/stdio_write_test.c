#include <errno.h>
#include <stddef.h>
#include <stdio.h>

struct scripted_write {
    int fd;
    long result;
};

struct scripted_read {
    long result;
    unsigned char byte;
};

static struct scripted_write write_script[16];
static size_t write_script_count;
static size_t write_script_index;
static struct scripted_read read_script[16];
static size_t read_script_count;
static size_t read_script_index;
static unsigned char output[64];
static size_t output_length;
static unsigned long counts[16];
static int write_fds[16];
static size_t write_call_count;
static size_t read_call_count;

static void reset_scripts(void)
{
    write_script_count = 0;
    write_script_index = 0;
    read_script_count = 0;
    read_script_index = 0;
    output_length = 0;
    write_call_count = 0;
    read_call_count = 0;
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

static void push_read(long result, unsigned char byte)
{
    read_script[read_script_count].result = result;
    read_script[read_script_count].byte = byte;
    ++read_script_count;
}

long mini_test_write(int fd, const void *buf, unsigned long count)
{
    const unsigned char *bytes = (const unsigned char *)buf;
    long result;
    size_t i;

    if (write_script_index >= write_script_count || write_call_count >= 16 ||
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

long mini_sys_read(int fd, void *buf, unsigned long count)
{
    long result;

    if (fd != 0 || count != 1 || read_script_index >= read_script_count ||
        read_call_count >= 16) {
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

int main(void)
{
    int result;

    reset_scripts();
    push_read(1, 'K');
    errno = ERANGE;
    result = fgetc(stdin);
    if (result != 'K' || errno != ERANGE || feof(stdin) || ferror(stdin) ||
        read_call_count != 1) {
        return 1;
    }

    reset_scripts();
    push_read(0, 0);
    errno = ERANGE;
    result = getc(stdin);
    if (result != EOF || errno != ERANGE || !feof(stdin) || ferror(stdin) ||
        read_call_count != 1) {
        return 2;
    }
    clearerr(stdin);
    if (feof(stdin) || ferror(stdin)) {
        return 3;
    }

    reset_scripts();
    push_read(-EINVAL, 0);
    errno = ERANGE;
    result = getchar();
    if (result != EOF || errno != EINVAL || feof(stdin) || !ferror(stdin) ||
        read_call_count != 1) {
        return 4;
    }
    clearerr(stdin);

    reset_scripts();
    errno = ERANGE;
    result = fgetc(stdout);
    if (result != EOF || errno != EINVAL || !ferror(stdout) || feof(stdout) ||
        read_call_count != 0) {
        return 5;
    }
    clearerr(stdout);

    reset_scripts();
    push_write(1, 2);
    push_write(1, 2);
    push_write(1, 1);
    errno = ERANGE;
    result = puts("abcd");
    if (result < 0 || errno != ERANGE || ferror(stdout) ||
        !bytes_equal("abcd\n", 5) || write_call_count != 3 ||
        counts[0] != 4 || counts[1] != 2 || counts[2] != 1 ||
        write_fds[0] != 1 || write_fds[1] != 1 || write_fds[2] != 1) {
        return 6;
    }

    reset_scripts();
    push_write(2, 2);
    push_write(2, 1);
    errno = ERANGE;
    result = fputs("xyz", stderr);
    if (result < 0 || errno != ERANGE || ferror(stderr) ||
        !bytes_equal("xyz", 3) || write_call_count != 2 ||
        write_fds[0] != 2 || write_fds[1] != 2 ||
        counts[0] != 3 || counts[1] != 1) {
        return 7;
    }

    reset_scripts();
    push_write(1, -EINVAL);
    errno = ERANGE;
    result = putchar('Z');
    if (result != EOF || errno != EINVAL || output_length != 0 ||
        write_call_count != 1 || counts[0] != 1 || !ferror(stdout)) {
        return 8;
    }

    push_write(1, 1);
    errno = ERANGE;
    result = fputc('Q', stdout);
    if (result != 'Q' || errno != ERANGE || !ferror(stdout) ||
        !bytes_equal("Q", 1)) {
        return 9;
    }
    clearerr(stdout);
    if (ferror(stdout) || feof(stdout)) {
        return 10;
    }

    reset_scripts();
    push_write(1, 2);
    push_write(1, -EINVAL);
    errno = ERANGE;
    result = fputs("abcd", stdout);
    if (result != EOF || errno != EINVAL || !ferror(stdout) ||
        !bytes_equal("ab", 2) || write_call_count != 2 ||
        counts[0] != 4 || counts[1] != 2) {
        return 11;
    }

    reset_scripts();
    push_write(1, 0);
    errno = ERANGE;
    result = fputc('R', stdout);
    if (result != EOF || errno != EIO || output_length != 0 ||
        write_call_count != 1 || counts[0] != 1 || !ferror(stdout)) {
        return 12;
    }

    reset_scripts();
    errno = ERANGE;
    result = fputc('S', stdin);
    if (result != EOF || errno != EINVAL || !ferror(stdin) ||
        write_call_count != 0) {
        return 13;
    }

    reset_scripts();
    errno = ERANGE;
    result = fputs("", stdout);
    if (result < 0 || errno != ERANGE || ferror(stdout) ||
        write_call_count != 0 || output_length != 0) {
        return 14;
    }

    return 0;
}
