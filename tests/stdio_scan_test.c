#include <errno.h>
#include <stddef.h>
#include <stdio.h>

static const unsigned char input[] =
    "1 2 3 4 5 6 word Q % skip 12X 0x2a 077 ff abcXYZ42_tail! @ ]-";
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
    int auto_hex = 0;
    int auto_oct = 0;
    unsigned int hex = 0;
    char word[5];
    char lower[4];
    char tail[6];
    char literal_set[3];
    char mismatch[2] = {'?', '\0'};
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

    if (scanf(" %i %i %x %3[a-z]%*[A-Z0-9]%5[^!]%*1[!]",
              &auto_hex, &auto_oct, &hex, lower, tail) != 5 ||
        auto_hex != 42 || auto_oct != 63 || hex != 255U ||
        lower[0] != 'a' || lower[1] != 'b' || lower[2] != 'c' ||
        lower[3] != '\0' || tail[0] != '_' || tail[1] != 't' ||
        tail[2] != 'a' || tail[3] != 'i' || tail[4] != 'l' ||
        tail[5] != '\0' || read_calls != 1) {
        return 5;
    }

    if (scanf(" %1[a-z]", mismatch) != 0 || mismatch[0] != '?' ||
        getchar() != '@' || read_calls != 1) {
        return 6;
    }
    if (scanf(" %2[]-]", literal_set) != 1 || literal_set[0] != ']' ||
        literal_set[1] != '-' || literal_set[2] != '\0' || read_calls != 1) {
        return 7;
    }

    first = 99;
    if (scanf("%d", &first) != EOF || first != 99 || !feof(stdin) ||
        ferror(stdin) || read_calls != 2) {
        return 8;
    }
    if (scanf("%d", &first) != EOF || read_calls != 2) {
        return 9;
    }

    return 0;
}
