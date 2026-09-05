#include <errno.h>
#include <stddef.h>
#include <stdio.h>

static const unsigned char input[] =
    "1 2 3 4 5 6 word Q % skip 12X";
static size_t input_offset;
static size_t read_calls;

long mini_test_read(int fd, void *buf, unsigned long count)
{
    unsigned char *out = (unsigned char *)buf;
    size_t remaining;
    size_t amount;
    size_t i;

    if (fd != 0) {
        return -EINVAL;
    }

    ++read_calls;
    if (input_offset == sizeof(input) - 1U) {
        return 0;
    }

    remaining = (sizeof(input) - 1U) - input_offset;
    amount = remaining;
    if (amount > (size_t)count) {
        amount = (size_t)count;
    }
    for (i = 0; i < amount; ++i) {
        out[i] = input[input_offset + i];
    }
    input_offset += amount;
    return (long)amount;
}

long mini_test_write(int fd, const void *buf, unsigned long count)
{
    (void)fd;
    (void)buf;
    (void)count;
    return -EIO;
}

int main(void)
{
    int values[6] = {0, 0, 0, 0, 0, 0};
    int first = 0;
    int second = 1234;
    char word[5];
    char character = '\0';

    errno = ERANGE;
    if (scanf("%d %d %d %d %d %d",
              &values[0], &values[1], &values[2], &values[3], &values[4],
              &values[5]) != 6 ||
        values[0] != 1 || values[1] != 2 || values[2] != 3 ||
        values[3] != 4 || values[4] != 5 || values[5] != 6 ||
        errno != ERANGE || read_calls != 1) {
        return 1;
    }

    if (scanf(" %4s %c %% %*s", word, &character) != 2 ||
        word[0] != 'w' || word[1] != 'o' || word[2] != 'r' ||
        word[3] != 'd' || word[4] != '\0' || character != 'Q' ||
        read_calls != 1) {
        return 2;
    }

    if (scanf("%d%d", &first, &second) != 1 || first != 12 ||
        second != 1234 || read_calls != 1) {
        return 3;
    }
    if (getchar() != 'X' || read_calls != 1) {
        return 4;
    }

    first = 99;
    if (scanf("%d", &first) != EOF || first != 99 || !feof(stdin) ||
        ferror(stdin) || read_calls != 2) {
        return 5;
    }
    if (scanf("%d", &first) != EOF || read_calls != 2) {
        return 6;
    }

    return 0;
}
