#include <errno.h>
#include <stddef.h>
#include <stdio.h>

struct scripted_write {
    long result;
};

static struct scripted_write script[16];
static size_t script_count;
static size_t script_index;
static unsigned char output[64];
static size_t output_length;
static unsigned long counts[16];
static size_t call_count;

static void reset_script(void)
{
    script_count = 0;
    script_index = 0;
    output_length = 0;
    call_count = 0;
}

static void push_result(long result)
{
    script[script_count++].result = result;
}

long mini_test_write(int fd, const void *buf, unsigned long count)
{
    const unsigned char *bytes = (const unsigned char *)buf;
    long result;
    size_t i;

    if (fd != 1 || script_index >= script_count || call_count >= 16) {
        return -EINVAL;
    }

    counts[call_count++] = count;
    result = script[script_index++].result;
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

    reset_script();
    push_result(2);
    push_result(2);
    push_result(1);
    errno = ERANGE;
    result = puts("abcd");
    if (result < 0 || errno != ERANGE || !bytes_equal("abcd\n", 5) ||
        call_count != 3 || counts[0] != 4 || counts[1] != 2 ||
        counts[2] != 1) {
        return 1;
    }

    reset_script();
    push_result(-EINVAL);
    errno = ERANGE;
    result = putchar('Z');
    if (result != EOF || errno != EINVAL || output_length != 0 ||
        call_count != 1 || counts[0] != 1) {
        return 2;
    }

    reset_script();
    push_result(2);
    push_result(-EINVAL);
    errno = ERANGE;
    result = puts("abcd");
    if (result != EOF || errno != EINVAL || !bytes_equal("ab", 2) ||
        call_count != 2 || counts[0] != 4 || counts[1] != 2) {
        return 3;
    }

    reset_script();
    push_result(0);
    errno = ERANGE;
    result = puts("x");
    if (result != EOF || errno != EIO || output_length != 0 ||
        call_count != 1 || counts[0] != 1) {
        return 4;
    }

    reset_script();
    push_result(0);
    errno = ERANGE;
    result = putchar('Q');
    if (result != EOF || errno != EIO || output_length != 0 ||
        call_count != 1 || counts[0] != 1) {
        return 5;
    }

    return 0;
}
