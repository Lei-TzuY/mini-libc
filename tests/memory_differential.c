#include <stddef.h>
#include <string.h>

#define BUF_SIZE 96
#define RANDOM_CASES 4000

void *mini_test_memcpy(void *restrict dest, const void *restrict src, size_t n);
void *mini_test_memmove(void *dest, const void *src, size_t n);
void *mini_test_memset(void *s, int c, size_t n);
int mini_test_memcmp(const void *s1, const void *s2, size_t n);
void *mini_test_memchr(const void *s, int c, size_t n);

static unsigned long rng_state = 0xd1b54a32d192ed03UL;

static unsigned long next_random(void)
{
    rng_state ^= rng_state << 7;
    rng_state ^= rng_state >> 9;
    rng_state ^= rng_state << 8;
    return rng_state;
}

static void fill_random(unsigned char *buf, size_t n)
{
    size_t i;
    for (i = 0; i < n; ++i) buf[i] = (unsigned char)next_random();
}

static int sign_of(int value)
{
    return (value > 0) - (value < 0);
}

int main(void)
{
    unsigned char source[BUF_SIZE];
    unsigned char mini_buf[BUF_SIZE];
    unsigned char host_buf[BUF_SIZE];
    int case_no;

    {
        unsigned char high = 0x80;
        unsigned char low = 0x7f;
        unsigned char search[5] = {'a',0,0xff,'a',0x80};
        if (sign_of(mini_test_memcmp(&high, &low, 1)) !=
            sign_of(memcmp(&high, &low, 1))) return 1;
        if (mini_test_memchr(search, 'a', 0) != memchr(search, 'a', 0) ||
            mini_test_memchr(search, 'a', sizeof(search)) != memchr(search, 'a', sizeof(search)) ||
            mini_test_memchr(search, 0, sizeof(search)) != memchr(search, 0, sizeof(search)) ||
            mini_test_memchr(search, -1, sizeof(search)) != memchr(search, -1, sizeof(search)) ||
            mini_test_memchr(search, 0x180, sizeof(search)) != memchr(search, 0x180, sizeof(search))) return 12;
    }

    for (case_no = 0; case_no < RANDOM_CASES; ++case_no) {
        size_t n;
        size_t src_off;
        size_t dst_off;
        int c;

        fill_random(source, BUF_SIZE);
        fill_random(mini_buf, BUF_SIZE);
        memcpy(host_buf, mini_buf, BUF_SIZE);
        n = (size_t)(next_random() % (BUF_SIZE + 1));
        src_off = (size_t)(next_random() % (BUF_SIZE - n + 1));
        dst_off = (size_t)(next_random() % (BUF_SIZE - n + 1));
        if (mini_test_memcpy(mini_buf + dst_off, source + src_off, n) != mini_buf + dst_off) return 2;
        if (memcpy(host_buf + dst_off, source + src_off, n) != host_buf + dst_off) return 3;
        if (memcmp(mini_buf, host_buf, BUF_SIZE) != 0) return 4;

        fill_random(mini_buf, BUF_SIZE);
        memcpy(host_buf, mini_buf, BUF_SIZE);
        n = (size_t)(next_random() % (BUF_SIZE + 1));
        dst_off = (size_t)(next_random() % (BUF_SIZE - n + 1));
        c = (int)next_random();
        if (mini_test_memset(mini_buf + dst_off, c, n) != mini_buf + dst_off) return 5;
        if (memset(host_buf + dst_off, c, n) != host_buf + dst_off) return 6;
        if (memcmp(mini_buf, host_buf, BUF_SIZE) != 0) return 7;

        fill_random(mini_buf, BUF_SIZE);
        memcpy(host_buf, mini_buf, BUF_SIZE);
        n = (size_t)(next_random() % (BUF_SIZE + 1));
        src_off = (size_t)(next_random() % (BUF_SIZE - n + 1));
        dst_off = (size_t)(next_random() % (BUF_SIZE - n + 1));
        if (mini_test_memmove(mini_buf + dst_off, mini_buf + src_off, n) != mini_buf + dst_off) return 8;
        if (memmove(host_buf + dst_off, host_buf + src_off, n) != host_buf + dst_off) return 9;
        if (memcmp(mini_buf, host_buf, BUF_SIZE) != 0) return 10;

        fill_random(mini_buf, BUF_SIZE);
        fill_random(host_buf, BUF_SIZE);
        n = (size_t)(next_random() % (BUF_SIZE + 1));
        if (sign_of(mini_test_memcmp(mini_buf, host_buf, n)) !=
            sign_of(memcmp(mini_buf, host_buf, n))) return 11;

        fill_random(source, BUF_SIZE);
        n = (size_t)(next_random() % (BUF_SIZE + 1));
        c = (int)(next_random() & 0x3ffUL) - 512;
        if (mini_test_memchr(source, c, n) != memchr(source, c, n)) return 13;
    }
    return 0;
}
